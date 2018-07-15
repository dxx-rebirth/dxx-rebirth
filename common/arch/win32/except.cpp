#include <array>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <windows.h>
#include <rpc.h>
#ifdef DXX_ENABLE_WINDOWS_MINIDUMP
#include <dbghelp.h>
#endif
#include "vers_id.h"

using path_buffer = std::array<wchar_t, MAX_PATH>;
void d_set_exception_handler();

namespace {

class RAII_Windows_FILE_HANDLE
{
	const HANDLE m_h;
public:
	RAII_Windows_FILE_HANDLE(HANDLE &&h) :
		m_h(h)
	{
	}
	~RAII_Windows_FILE_HANDLE()
	{
		if (*this)
			CloseHandle(m_h);
	}
	RAII_Windows_FILE_HANDLE(const RAII_Windows_FILE_HANDLE &) = delete;
	RAII_Windows_FILE_HANDLE &operator=(const RAII_Windows_FILE_HANDLE &) = delete;
	explicit operator bool() const
	{
		return m_h != INVALID_HANDLE_VALUE;
	}
	operator HANDLE() const
	{
		return m_h;
	}
};

class RAII_Windows_DynamicSharedObject
{
	HMODULE m_h;
	RAII_Windows_DynamicSharedObject(HMODULE &&h) :
		m_h(h)
	{
	}
public:
	~RAII_Windows_DynamicSharedObject()
	{
		if (*this)
			FreeLibrary(m_h);
	}
	RAII_Windows_DynamicSharedObject(const RAII_Windows_DynamicSharedObject &) = delete;
	RAII_Windows_DynamicSharedObject &operator=(const RAII_Windows_DynamicSharedObject &) = delete;
	RAII_Windows_DynamicSharedObject(RAII_Windows_DynamicSharedObject &&) = default;
	explicit operator bool() const
	{
		return m_h;
	}
	/*
	 * This leaks the loaded library.  It is used to ensure the library
	 * will still be loaded when the termination handler runs, long
	 * after this object has gone out of scope.
	 */
	void release()
	{
		m_h = nullptr;
	}
	template <std::size_t N>
		static RAII_Windows_DynamicSharedObject Load(std::array<wchar_t, MAX_PATH> &pathbuf, const unsigned lws, const wchar_t (&filename)[N])
		{
			if (lws >= MAX_PATH - N)
				return nullptr;
			wcscpy(&pathbuf[lws], filename);
			return LoadLibraryW(pathbuf.data());
		}
	template <typename T>
		T *GetProc(const char *const proc) const
		{
			union {
				FARPROC gpa;
				T *result;
			};
			gpa = GetProcAddress(m_h, proc);
			return result;
		}
};

struct dxx_trap_context
{
	EXCEPTION_RECORD ExceptionRecord;
	CONTEXT ContextRecord;
	/* Pointer to the first inaccessible byte above the stack. */
	const uint8_t *end_sp;
};

}

#define DXX_REPORT_TEXT_FORMAT_UUID	"Format UUID: 0f7deda4-1122-4be8-97d5-afc91b7803d6"
#define DXX_REPORT_TEXT_LEADER_VERSION_PREFIX	"Rebirth version: x"
#define DXX_REPORT_TEXT_LEADER_BUILD_DATETIME	"Rebirth built: "
#define DXX_REPORT_TEXT_LEADER_EXCEPTION_MESSAGE	 "Exception message: "

constexpr unsigned dump_stack_bytes = 2048 * sizeof(void *);
constexpr unsigned dump_stack_stride = 16;

/*
 * This includes leading text describing the UUID, so it must be bigger
 * than a bare UUID would require.
 */
static std::array<char, 54> g_strctxuuid;
/*
 * These labels are defined in asm() statements to record the address at
 * which special instructions are placed.  The array size must match the
 * size of the assembly instruction.  See `vectored_exception_handler`
 * for how these are used.
 */
extern const char dxx_rebirth_veh_ud2[2], dxx_rebirth_veh_sp[2];

/*
 * 64 bit registers use an R prefix; 32 bit registers use an E prefix.
 * For this handler, they can be treated the same.  Use a macro to
 * expand to the proper name.
 *
 * Win32 uses a leading underscore on names, relative to the C name.
 * Win64 does not.  Use a macro to cover that difference.
 */
#ifdef WIN64
#define DXX_NT_CONTEXT_REGISTER(n)	R##n
#define DXX_ASM_LABEL(L)	"" L
#define DXX_WINDOWS_HOST_ARCH_W	L"64"
#define DXX_REPORT_TEXT_LEADER_VERSION	DXX_REPORT_TEXT_LEADER_VERSION_PREFIX "64"
#else
#define DXX_NT_CONTEXT_REGISTER(n)	E##n
#define DXX_ASM_LABEL(L)	"_" L
#define DXX_WINDOWS_HOST_ARCH_W	L"86"
#define DXX_REPORT_TEXT_LEADER_VERSION	DXX_REPORT_TEXT_LEADER_VERSION_PREFIX "86"
#endif

static LONG CALLBACK vectored_exception_handler(EXCEPTION_POINTERS *const ep)
{
	const auto ctx = ep->ContextRecord;
	auto &ip = ctx->DXX_NT_CONTEXT_REGISTER(ip);
	auto &ax = ctx->DXX_NT_CONTEXT_REGISTER(ax);
	const auto t = reinterpret_cast<dxx_trap_context *>(ax);
	/*
	 * If the fault address is dxx_rebirth_veh_ud2, this is an expected
	 * fault forced solely to capture register context.  This fault must
	 * always happen.
	 */
	if (ip == reinterpret_cast<uintptr_t>(dxx_rebirth_veh_ud2))
	{
		/*
		 * Copy the captured data into the faulting function's local
		 * variable.  The address of that local was loaded into ax before
		 * triggering the fault.
		 */
		t->ExceptionRecord = *ep->ExceptionRecord;
		t->ContextRecord = *ctx;
		/* Step past the faulting instruction. */
		ip += sizeof(dxx_rebirth_veh_ud2);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	/*
	 * If the fault address is dxx_rebirth_veh_sp, this is an expected
	 * fault triggered by scanning up off the top of the stack.  This
	 * fault usually happens, but might not if the exception happened
	 * with a sufficiently deep stack.
	 */
	else if (ip == reinterpret_cast<uintptr_t>(dxx_rebirth_veh_sp))
	{
		/*
		 * The faulting function arranged for ax to hold the terminating
		 * address of the search and for bx to hold the currently tested
		 * address.  When the fault happens, bx points to an
		 * inaccessible byte.  Set ax to point to that byte.  Decrement
		 * bx to counter the guaranteed increment in the faulting
		 * function.  The combination of these changes ensures that the
		 * (current != end) test fails, terminating the loop without
		 * provoking an additional fault.
		 */
		ax = ctx->DXX_NT_CONTEXT_REGISTER(bx)--;
		/* Step past the faulting instruction. */
		ip += sizeof(dxx_rebirth_veh_sp);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

/* Ensure gcc does not clone the inline labels */
__attribute__((__noinline__,__noclone__,__warn_unused_result__))
static void *capture_exception_context(dxx_trap_context &dtc, const uint8_t *const sp)
{
	const auto handler = AddVectoredExceptionHandler(1, &vectored_exception_handler);
	if (handler)
	{
		dtc = {};
		/*
		 * This block guarantees at least one and at most two faults to
		 * occur.  Run it only if an exception handler was successfully
		 * added to catch these faults.
		 */
		asm volatile(
DXX_ASM_LABEL("dxx_rebirth_veh_ud2") ":\n"
/*
 * Force a fault by executing a guaranteed undefined instruction.  The
 * fault handler will capture register context for a minidump, then step
 * past this instruction.
 *
 * Before the fault, store the address of the `dxx_trap_context`
 * instance `dtc` into ax for easy access by the fault handler.  The
 * fault handler then initializes dtc from the captured data.  Mark
 * memory as clobbered to prevent any speculative caching of `dtc`
 * across the asm block.  A narrower clobber could be used, but "memory"
 * is sufficient and this code is not performance critical.
 */
"	ud2\n"
			:: "a" (&dtc) : "memory"
		);
		auto p = reinterpret_cast<uintptr_t>(sp);
		for (auto e = p + dump_stack_bytes; p != e; ++p)
			asm volatile(
DXX_ASM_LABEL("dxx_rebirth_veh_sp") ":\n"
/*
 * Force a read of the address pointed at by [sp].  This must be done in
 * assembly, not through a volatile C read, because the vectored
 * exception handler must know the specific address of the read
 * instruction.
 *
 * This may fault if the stack is not sufficiently deep.  If a fault
 * happens, the fault handler will adjust `e` and `p`, then step past
 * the compare (although stepping past is not strictly necessary).  On
 * resume from the fault, the loop will find that the adjusted values
 * make `++p` == `e`, causing the loop to terminate.
 */
"	cmpb %%al,(%[sp])\n"
				: "+a" (e),
				[sp] "+b" (p)
				:: "cc"
			);
		/*
		 * Save the address of the first inaccessible byte, rounded down
		 * to the nearest paragraph.
		 */
		dtc.end_sp = reinterpret_cast<const uint8_t *>(p & -dump_stack_stride);
		RemoveVectoredExceptionHandler(handler);
	}
	return handler;
}
#undef DXX_ASM_LABEL

/*
 * Initialize `path` with a path suitable to be used to write a minidump
 * for the exception.  Initialize `filename` to point to the first
 * character in the filename (bypassing the directories).
 */
static unsigned prepare_exception_log_path(path_buffer &path, wchar_t *&filename, const unsigned pid)
{
	const auto l = GetTempPathW(path.size(), path.data());
	SYSTEMTIME st{};
	GetSystemTime(&st);
	/*
	 * Subsecond precision is not required.  The program will terminate
	 * after writing this dump.  No user is likely to be able to restart
	 * the game, and provoke a second dump, and on the same PID, within
	 * the same second.
	 */
	const auto r = l + _snwprintf(filename = path.data() + l, path.size() - l, L"%.4u-%.2u-%.2u-%.2u-%.2u-%.2u x" DXX_WINDOWS_HOST_ARCH_W L" P%u %hs.elog", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, pid, g_descent_version);
	const auto extension = path.data() + r - 5;
	wchar_t *p;
	/*
	 * NTFS does not allow star in filenames.  The expected content of
	 * this part of the filename will never contain an 's', so replace
	 * '*' with 's'.  The filename will contain at most one star, but
	 * possibly none if the version string reported no unstaged changes.
	 * The position of the star varies depending on whether the version
	 * string also included a plus to report staged uncommitted changes.
	 */
	if (*(p = &extension[-1]) == '*' ||
		*(p = &extension[-2]) == '*')
		*p = 's';
	return r;
}

#ifdef DXX_ENABLE_WINDOWS_MINIDUMP
using MiniDumpWriteDump_type = decltype(MiniDumpWriteDump);
static MiniDumpWriteDump_type *g_pMiniDumpWriteDump;

/*
 * Write a minidump to the open file `h`, reported as being from pid
 * `pid` with context from `dtc`.  If `what` is not empty, include it as
 * a comment stream.  The caller uses this to pass the exception text
 * (as returned by `std::exception::what()`) so that it will be easy to
 * find in the debugger.
 */
static void write_exception_dump(dxx_trap_context &dtc, const unsigned pid, const std::string &what, const HANDLE h, const MiniDumpWriteDump_type *const pMiniDumpWriteDump)
{
	MINIDUMP_EXCEPTION_INFORMATION mei;
	MINIDUMP_USER_STREAM_INFORMATION musi;
	EXCEPTION_POINTERS ep;
	ep.ContextRecord = &dtc.ContextRecord;
	ep.ExceptionRecord = &dtc.ExceptionRecord;
	mei.ThreadId = GetCurrentThreadId();
	mei.ExceptionPointers = &ep;
	mei.ClientPointers = 0;
	std::array<MINIDUMP_USER_STREAM, 5> mus;
	std::array<char, 92> musTextVersion;
	std::array<char, 44> musTextDateTime;
	std::array<char, 512> musTextExceptionMessage;
	unsigned UserStreamCount = 0;
	{
		auto &m = mus[UserStreamCount++];
		m.Type = MINIDUMP_STREAM_TYPE::CommentStreamA;
		static char leader[] = DXX_REPORT_TEXT_FORMAT_UUID;
		m.BufferSize = sizeof(leader);
		m.Buffer = leader;
	}
	{
		auto &m = mus[UserStreamCount++];
		m.Type = MINIDUMP_STREAM_TYPE::CommentStreamA;
		m.BufferSize = g_strctxuuid.size();
		m.Buffer = g_strctxuuid.data();
	}
	{
		auto &m = mus[UserStreamCount++];
		m.Type = MINIDUMP_STREAM_TYPE::CommentStreamA;
		m.Buffer = musTextVersion.data();
		m.BufferSize = 1 + std::snprintf(musTextVersion.data(), musTextVersion.size(), DXX_REPORT_TEXT_LEADER_VERSION " %s", g_descent_version);
	}
	{
		auto &m = mus[UserStreamCount++];
		m.Type = MINIDUMP_STREAM_TYPE::CommentStreamA;
		m.Buffer = musTextDateTime.data();
		m.BufferSize = 1 + std::snprintf(musTextDateTime.data(), musTextDateTime.size(), DXX_REPORT_TEXT_LEADER_BUILD_DATETIME "%s", g_descent_build_datetime);
	}
	if (!what.empty())
	{
		auto &m = mus[UserStreamCount++];
		m.Type = MINIDUMP_STREAM_TYPE::CommentStreamA;
		m.Buffer = musTextExceptionMessage.data();
		m.BufferSize = 1 + std::snprintf(musTextExceptionMessage.data(), musTextExceptionMessage.size(), DXX_REPORT_TEXT_LEADER_EXCEPTION_MESSAGE "%s", what.c_str());
	}
	musi.UserStreamCount = UserStreamCount;
	musi.UserStreamArray = mus.data();
	(*pMiniDumpWriteDump)(GetCurrentProcess(), pid, h, MiniDumpWithFullMemory, &mei, &musi, nullptr);
}

/*
 * Open the file named by `path` for write.  On success, call
 * `write_exception_dump` above with that handle.  On failure, clear the
 * path so that the later UI code does not tell the user about a file
 * that does not exist.
 */
static void write_exception_dump(dxx_trap_context &dtc, const unsigned pid, const std::string &what, path_buffer &path, const MiniDumpWriteDump_type *const pMiniDumpWriteDump)
{
	if (RAII_Windows_FILE_HANDLE h{CreateFileW(path.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)})
		write_exception_dump(dtc, pid, what, h, pMiniDumpWriteDump);
	else
		path.front() = 0;
}
#endif

/*
 * Write a plain text dump of metadata and a hex dump of the faulting
 * stack.
 */
static void write_exception_stack(const wchar_t *const filename, const unsigned pid, const uint8_t *const begin_sp, const dxx_trap_context &dtc, const std::string &what, const HANDLE h)
{
	std::array<char, 4096> buf;
	buf = {};
	const auto len_header_text = std::snprintf(buf.data(), buf.size(),
"Rebirth exception printed context\n"
DXX_REPORT_TEXT_FORMAT_UUID "\n"
"%.50s\n"
DXX_REPORT_TEXT_LEADER_VERSION " %s\n"
DXX_REPORT_TEXT_LEADER_BUILD_DATETIME "%s\n"
"Rebirth PID: %u\n"
"Report date-time: %.19ls\n"
DXX_REPORT_TEXT_LEADER_EXCEPTION_MESSAGE "\"%s\"\n"
"UD2 IP: %p\n"
"SP: %p\n"
"\n"
, g_strctxuuid.data(), g_descent_version, g_descent_build_datetime, pid, filename, what.c_str(), reinterpret_cast<const uint8_t *>(dtc.ContextRecord.DXX_NT_CONTEXT_REGISTER(ip)), begin_sp
);
	/*
	 * end_sp is rounded down to a paragraph boundary.
	 * Round sp the same way so that there will exist an integer N such
	 * that (`sp` + (N * `dump_stack_stride`)) == `end_sp`.
	 */
	const auto sp = reinterpret_cast<const uint8_t *>(reinterpret_cast<uintptr_t>(begin_sp) & -dump_stack_stride);
	DWORD dwWritten;
	WriteFile(h, buf.data(), len_header_text, &dwWritten, 0);
	const auto end_sp = dtc.end_sp;
	for (unsigned i = 0; i < dump_stack_bytes; i += dump_stack_stride)
	{
		char hexdump[dump_stack_stride + 1];
		const auto base_paragraph_pointer = &sp[i];
		if (base_paragraph_pointer == end_sp)
			break;
		hexdump[dump_stack_stride] = 0;
		for (unsigned j = dump_stack_stride; j--;)
		{
			const uint8_t c = reinterpret_cast<const uint8_t *>(base_paragraph_pointer)[j];
			hexdump[j] = (c >= ' ' && c <= '~') ? c : '.';
		}
		buf = {};
#define FORMAT_PADDED_UINT8_4	" %.2x %.2x %.2x %.2x "
#define VAR_PADDED_UINT8_4(I)	sp[I], sp[I + 1], sp[I + 2], sp[I + 3]
		const auto len_line_text = std::snprintf(buf.data(), buf.size(),
"%p:  " FORMAT_PADDED_UINT8_4 FORMAT_PADDED_UINT8_4 FORMAT_PADDED_UINT8_4 FORMAT_PADDED_UINT8_4 "    %s\n"
, base_paragraph_pointer, VAR_PADDED_UINT8_4(i), VAR_PADDED_UINT8_4(i + 4), VAR_PADDED_UINT8_4(i + 8), VAR_PADDED_UINT8_4(i + 12), hexdump);
#undef VAR_PADDED_UINT8_4
#undef FORMAT_PADDED_UINT8_4
		WriteFile(h, buf.data(), len_line_text, &dwWritten, 0);
	}
}

static void write_exception_stack(const wchar_t *const filename, const unsigned pid, const uint8_t *const sp, const dxx_trap_context &dtc, const std::string &what, path_buffer &path)
{
	if (RAII_Windows_FILE_HANDLE h{CreateFileW(path.data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)})
		write_exception_stack(filename, pid, sp, dtc, what, h);
	else
		path.front() = 0;
}

/*
 * Prevent moving the large `dxx_trap_context` into the caller.
 * The caller captures the stack pointer before this function begins.
 * The captured value should exclude the locals of this function, so
 * that the data printed from the captured value covers the stack used
 * prior to the throw which triggered the terminate handler.  If this
 * function were inlined into the caller, the captured stack pointer
 * would include the locals of this function.
 */
__attribute__((__noinline__,__noclone__))
static void write_exception_logs(const uint8_t *const sp)
{
	std::string what;
	try {
		/*
		 * Rethrow the faulting exception, then catch it to extract its
		 * explanatory text.
		 */
		std::rethrow_exception(std::current_exception());
	} catch (const std::exception &ee) {
		what = ee.what();
	} catch (...) {
	}
#ifdef DXX_ENABLE_WINDOWS_MINIDUMP
	path_buffer path_dump;
#define DXX_PATH_DUMP_FORMAT_STRING	L"%s%s\n"
#define DXX_PATH_DUMP_ARGUMENTS	\
	, path_dump.front() ? L"\nIf possible, make available the binary file:\n  " : L""	\
	, path_dump.data()
#else
#define DXX_PATH_DUMP_FORMAT_STRING
#define DXX_PATH_DUMP_ARGUMENTS
#endif
	path_buffer path_stack;
	dxx_trap_context dtc;
	if (capture_exception_context(dtc, sp))
	{
		const unsigned pid = GetCurrentProcessId();
		wchar_t *filename;
		const auto pl = prepare_exception_log_path(path_stack, filename, pid);
#ifdef DXX_ENABLE_WINDOWS_MINIDUMP
		path_dump = path_stack;
		wcscpy(&path_dump[pl - 4], L"mdmp");
		if (const auto pMiniDumpWriteDump = g_pMiniDumpWriteDump)
			write_exception_dump(dtc, pid, what, path_dump, pMiniDumpWriteDump);
		else
			path_dump.front() = 0;
#else
		(void)pl;
#endif
		write_exception_stack(filename, pid, sp, dtc, what, path_stack);
	}
	else
	{
#ifdef DXX_ENABLE_WINDOWS_MINIDUMP
		path_dump.front() = 0;
#endif
		path_stack.front() = 0;
	}
	std::array<wchar_t, 1024> msg;
	_snwprintf(msg.data(), msg.size(),
L"Rebirth encountered a fatal error.  Please report this to the developers.\n"
L"\nInclude in your report:\n"
L"%s%hs%s"
L"* The level(s) played this session, including download URLs for any add-on missions\n"
L"%s%s\n"
DXX_PATH_DUMP_FORMAT_STRING
L"\nTo the extent possible, provide steps to reproduce, starting from the game main menu.",
what.empty() ? L"" : L"* The exception message text:\n  \"", what.c_str(), what.empty() ? L"" : L"\"\n",
path_stack.front() ? L"* The contents of the text file:\n  " : L"", path_stack.data()
DXX_PATH_DUMP_ARGUMENTS
);
#undef DXX_PATH_DUMP_ARGUMENTS
#undef DXX_PATH_DUMP_FORMAT_STRING
	MessageBoxW(NULL, msg.data(), L"Rebirth - Fatal Error", MB_ICONERROR | MB_TOPMOST | MB_TASKMODAL);
}

[[noreturn]]
static void terminate_handler()
{
	const uint8_t *sp;
	asm(
#ifdef WIN64
		"movq %%rsp, %[sp]"
#else
		"movl %%esp, %[sp]"
#endif
		: [sp] "=rm" (sp));
	write_exception_logs(sp);
	ExitProcess(0);
}

void d_set_exception_handler()
{
	std::set_terminate(&terminate_handler);
	std::array<wchar_t, MAX_PATH> ws;
	if (const auto lws = GetSystemDirectoryW(ws.data(), ws.size()))
	{
		if (auto &&rpcrt4 = RAII_Windows_DynamicSharedObject::Load(ws, lws, L"\\rpcrt4.dll"))
		{
			if (const auto pUuidCreate = rpcrt4.GetProc<decltype(UuidCreate)>("UuidCreate"))
			{
				UUID ctxuuid;
				/*
				 * Create a UUID specific to this run.
				 */
				if (SUCCEEDED((*pUuidCreate)(&ctxuuid)))
				{
					std::snprintf(g_strctxuuid.data(), g_strctxuuid.size(), "Context UUID: %08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", ctxuuid.Data1, ctxuuid.Data2, ctxuuid.Data3, ctxuuid.Data4[0], ctxuuid.Data4[1], ctxuuid.Data4[2], ctxuuid.Data4[3], ctxuuid.Data4[4], ctxuuid.Data4[5], ctxuuid.Data4[6], ctxuuid.Data4[7]);
				}
			}
		}
#ifdef DXX_ENABLE_WINDOWS_MINIDUMP
		if (auto &&dbghelp = RAII_Windows_DynamicSharedObject::Load(ws, lws, L"\\dbghelp.dll"))
		{
			if ((g_pMiniDumpWriteDump = dbghelp.GetProc<decltype(MiniDumpWriteDump)>("MiniDumpWriteDump")))
				dbghelp.release();
		}
#endif
	}
}

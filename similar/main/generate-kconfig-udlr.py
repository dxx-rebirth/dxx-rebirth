#!/usr/bin/python

import ast, os, re, sys

# Storage for the relevant fields from an initializer for a single
# element of a kc_item array.  Fields not required for sorting or
# generating the output header are not captured or saved.
class ArrayInitializationLine:
	def __init__(self,xinput,y,name,enum,label,idx):
		# xinput, y are fields in the kc_item structure
		self.xinput = xinput
		self.y = y
		# name is the token that will be preprocessor-defined to the
		# appropriate udlr values
		self.name = name
		self.enum = enum
		self.label = label
		# index in the kc_item array at which this initializer was found
		self.idx = idx

	# For horizontal sorting, group elements by their vertical
	# coordinate, then break ties using the horizontal coordinate.  This
	# causes elements from a single row to be contiguous in the sorted
	# output, and places visually adjacent rows adjacent in the sorted
	# output.
	def key_horizontal(self):
		return (self.y, self.xinput)

	# For vertical sorting, group elements by their horizontal
	# coordinate, then break ties using the vertical coordinate.
	def key_vertical(self):
		return (self.xinput, self.y)

# InputException is raised when the post-processed C++ table cannot be
# parsed using the regular expressions in this script.  The table may or
# may not be valid C++ when this script rejects it.
class InputException(Exception):
	pass

# NodeVisitor handles walking a Python Abstract Syntax Tree (AST) to
# emulate a few supported operations, and raise an error on anything
# unsupported.  This is used to implement primitive support for
# evaluating arithmetic expressions in the table.  This support covers
# only what is likely to be used and assumes Python syntax (which should
# usually match C++ syntax closely enough, for the limited forms
# supported).
class NodeVisitor(ast.NodeVisitor):
	def __init__(self,source,lno,name,expr,constants):
		self.source = source
		self.lno = lno
		self.name = name
		self.expr = expr
		self.constants = constants

	def generic_visit(self,node):
		raise InputException(f'{self.source}:{self.lno}: {self.name!r} expression {self.expr!r} uses unsupported node type {node.__class__.__name__}')

	def visit_BinOp(self,node):
		left = self.visit(node.left)
		right = self.visit(node.right)
		op = node.op
		if isinstance(op, ast.Add):
			return left + right
		elif isinstance(op, ast.Sub):
			return left - right
		elif isinstance(op, ast.Mult):
			return left * right
		elif isinstance(op, ast.Div):
			return left / right
		else:
			raise InputException(f'{self.source}:{self.lno}: {self.name!r} expression {self.expr!r} uses unsupported BinOp node type {op.__class__.__name__}')

	# Resolve expressions by expanding them.
	def visit_Expression(self,node):
		return self.visit(node.body)

	# Resolve names by searching the dictionary `constants`, which is
	# initialized from C++ constexpr declarations found while scanning
	# the file.
	def visit_Name(self,node):
		try:
			return self.constants[node.id]
		except KeyError as e:
			raise InputException(f'{self.source}:{self.lno}: {self.name!r} expression {self.expr!r} uses undefined name {node.id}' % (self.source, self.lno, self.name, self.expr, node.id))

	# Resolve numbers by returning the value as-is.
	@staticmethod
	def visit_Num(node):
		return node.n

class Main:
	def __init__(self):
		# Initially, no constants are known.  Elements will be added by
		# prepare_text.
		self.constants = {}

	# Resolve an expression by parsing it into an AST, then visiting
	# the nodes and emulating the permitted operations.  Any
	# disallowed operations will cause an exception, which will
	# propagate through resolve_expr into the caller.
	def resolve_expr(self,source,lno,name,expr):
		expr = expr.strip()
		return NodeVisitor(source, lno, name, expr, self.constants).visit(ast.parse(expr, mode='eval'))

	# Given a list of C++ initializer lines, extract from those lines
	# the position of each initialized cell, compute the u/d/l/r
	# relations among the cells, and return a multiline string
	# containing CPP #define statements appropriate to the cells.  Each
	# C++ initializer must be entirely contained within a single element
	# in `lines`.
	#
	# array_name - C++ identifier for the array; used in a C
	# comment and, if labels are used, as part of the name of the macro
	# that expands to the label
	#
	# source - name of the file from which the lines were read
	#
	# lines - iterable of initializer lines
	#
	# _re_finditer_init_element - bound method for a regular expression to
	# extract the required fields
	def generate_defines(self,array_name,source,lines,_re_finditer_init_element=re.compile(r'{'
			r'(?:[^,]+),'	# x
			r'(?P<y>[^,]+),'
			r'(?P<xinput>[^,]+),'
			r'(?:[^,]+),'	# w2
			r'\s*(?:DXX_KCONFIG_UI_ENUM\s*\((?P<enum>\w+)\)\s*)?(?:DXX_KCONFIG_UI_LABEL\s*\((?P<label>(?:"[^"]*")|\w*)\)\s*)?(?P<udlr>\w+)\(\),'
			r'\s*(?:[^,]+,'	# type
				r'[^,]+,'	# state_bit
				r'[^,]+)'	# state_ptr
			r'},').finditer):
		a = ArrayInitializationLine
		array = []
		append = array.append
		resolve_expr = self.resolve_expr
		idx = 0
		# Iterate over the initializer lines and populate `array` with
		# the extracted data.
		for lno, line in lines:
			ml = _re_finditer_init_element(line)
			old_idx = idx
			for m in ml:
				m = m.group
				append(a(
					resolve_expr(source, lno, 'xinput', m('xinput')),
					resolve_expr(source, lno, 'y', m('y')),
					m('udlr'), m('enum'), m('label'), idx))
				idx = idx + 1
			# If the loop executes zero times, then the regular
			# expression failed to match, `old_idx is idx` is True, and
			# an exception should be raised, because well-formed input
			# will always match at least once.
			#
			# If the loop executes at least once, then a result was
			# found, not `old_idx is idx`, and no exception is necessary.
			if old_idx is idx:
				raise InputException(f'{source}:{lno}: failed to match regex for line {line!r}\n')
		if not array:
			# An empty array is not useful, but may exist when the
			# developer is adding a new array and has not yet defined
			# any elements.
			return f'\n/* {array_name} - udlr blank */',
		# Generate a temporary list with the elements sorted for u/d
		# navigation.  Walk the temporary and assign appropriate u/d
		# references.
		s = sorted(array, key=a.key_vertical)
		p = s[-1]
		for i in s:
			i.next_u = p
			p.next_d = i
			p = i
		# As above, but sorted for l/r navigation.
		s = sorted(array, key=a.key_horizontal)
		p = s[-1]
		for i in s:
			i.next_l = p
			p.next_r = i
			p = i
		define_enum_footer = '};'
		enum = []
		label = []
		# Generate the `#define` lines using the relations computed by
		# the preceding loops.  Both ordering loops must execute
		# completely before the next_* data required by the udlr
		# `#define` lines is available.  The udlr logic cannot be folded
		# into a prior loop.
		#
		# The enum member and label logic could be handled in a prior
		# loop, but are more logical here.  Moving them to an earlier
		# loop offers no gain.
		result = [f'/* {array_name} - udlr define */']
		for i in array:
			idx = i.idx
			# This must be a `#define` since it expands to a comma-separated
			# list of values for a structure initializer.
			result.append(f'#define {i.name}()\t/* [{idx:2d}] */\t{i.next_u.idx:2d},{i.next_d.idx:3d},{i.next_l.idx:3d},{i.next_r.idx:3d}')
			il = i.enum
			if il:
				# Generate `#define` statements to allow the consuming code to
				# use `#ifdef` to detect which members exist.
				enum.append(f'''\
#define dxx_kconfig_ui_{array_name}_{il} dxx_kconfig_ui_{array_name}::{il}
	{il} = {idx},''')
			il = i.label
			if il:
				label.append(f'\t\\\n\t/* [{idx:2d}] */\t{il} "\\0"')
		if enum:
			result.append(f'\n/* {array_name} - enum define */')
			# Use an `enum class` here so that the values can only be used in
			# arrays specifically marked for this index type.  This prevents
			# mixing types, such as indexing the mouse array with joystick
			# index values.
			result.append(f'enum class dxx_kconfig_ui_{array_name} : unsigned {{')
			result.extend(enum)
			result.append(define_enum_footer)
		else:
			result.append(f'\n/* {array_name} - enum blank */')
		if label:
			result.append(f'\n#define DXX_KCONFIG_UI_LABEL_{array_name}{"".join(label)}\n')
		return result

	# Given an iterable over a CPP-processed C++ file, find each kc_item
	# array in the file, generate appropriate #define statements, and
	# return the statements as a multiline string.
	#
	# script - path to this script
	#
	# fi - iterable over the CPP-processed C++ file
	#
	# _re_match_defn_const - bound match method for a regular expression
	# to extract a C++ expression from the initializer of a
	# std::integral_constant.
	#
	# _re_match_defn_array - bound match method for a regular expression
	# to match the opening definition of a C++ kc_item array.
	def prepare_text(self,script,fi,
		_re_match_defn_const=re.compile(r'constexpr\s+std::integral_constant<\w+\s*,\s*((?:\w+\s*\+\s*)*\w+)>\s*(\w+){};').match,
		_re_match_defn_array=re.compile(r'constexpr\s+kc_item\s+(\w+)\s*\[\]\s*=\s*{').match
		):
		source = fi.name
		result = [f'''/* This is a generated file.  Do not edit.
 * This file was generated by {script}
 * This file was generated from {source}
 */
''']
		lines = []
		# Simple line reassembly is done automatically, based on the
		# requirement that braces be balanced to complete an
		# initializer.  This is required to handle cases where the
		# initializer split onto multiple lines due to an embedded
		# preprocessor directive.
		unbalanced_open_brace = 0
		array_name = None
		partial_line = None
		generate_defines = self.generate_defines
		for lno, line in enumerate(fi, 1):
			if line.startswith('#'):
				# Ignore line/column position information
				continue
			line = line.strip()
			if not line:
				# Ignore blank lines
				continue
			if line == '};':
				# End of array found.  Check for context errors.
				# Compute #define statements for this array.  Reset for
				# the next array.
				if array_name is None:
					raise InputException(f'{source}:{lno}: end of array definition while no array open')
				if unbalanced_open_brace:
					raise InputException(f'{source}:{lno}: end of array definition while reading array initialization')
				result.extend(generate_defines(array_name, source, lines))
				lines = []
				array_name = None
				continue
			if array_name is None:
				# These expressions should never match outside an array
				# definition, so apply them only when an array is open.
				m = _re_match_defn_const(line)
				if m is not None:
					# Record a C++ std::integral_constant for later use
					# evaluating table cells.
					g = m.group
					self.constants[g(2)] = self.resolve_expr(source, lno, 'constant', g(1))
					continue
				m = _re_match_defn_array(line)
				if m is not None:
					# Array definition found.
					array_name = m.group(1)
					continue
			count = line.count
			unbalanced_open_brace += count('{') - count('}')
			if unbalanced_open_brace < 0:
				raise InputException(f'{source}:{lno}: brace count becomes negative')
			if partial_line is not None:
				# Insert a fake whitespace to avoid combining a token at
				# the end of one line with a token at the beginning of
				# the next.
				line = partial_line + ' ' + line
			if unbalanced_open_brace:
				# If braces are unbalanced, assume that this line is
				# incomplete, save it for later, and proceed to the next
				# line.
				partial_line = line
				continue
			partial_line = None
			lines.append((lno, line))
		# Check for context error.
		if array_name is not None:
			raise InputException(f'{source}: end of file while array definition open')
		# Ensure end-of-line at end-of-file.
		result.append('')
		return '\n'.join(result)

	@staticmethod
	def write_generated_text(target,generated_text):
		if not target:
			# As a special case, allow this script to be used as a
			# filter.
			sys.stdout.write(generated_text)
			return
		from tempfile import mkstemp
		os_path = os.path
		fd, path = mkstemp(suffix='', prefix=f'{os_path.basename(target)}.', dir=os_path.dirname(target), text=True)
		os.write(fd, generated_text.encode())
		os.close(fd)
		os.rename(path, target)

	def main(self,script,source,target):
			# Read the entire file and prepare the output text before
			# opening the output file.
		try:
			with (open(source, 'r') if source else sys.stdin) as fi:
				generated_text = self.prepare_text(script,fi)
		except InputException as e:
			# Normally, input exceptions are presented as a simple
			# error message.  If this environment variable is set,
			# show the full traceback.
			if os.getenv('DXX_KCONFIG_UDLR_TRACEBACK') is not None:
				raise
			sys.stderr.write(f'error: {e!s}\n')
			sys.exit(1)
		self.write_generated_text(target,generated_text)

if __name__ == '__main__':
	a = sys.argv
	# As a convenience feature, if no input filename is given, read
	# stdin.  If no output filename is given, write to stdout.
	l = len(a)
	if l < 2:
		a.append(None)
	if l < 3:
		a.append(None)
	Main().main(*a[0:3])

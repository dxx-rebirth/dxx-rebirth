#!/usr/bin/python3

class ReverseMapping(dict):
	def re_sub(self, match):
		i = int(match.group(2), 16)
		symbolic = self.get(i, self)
		if symbolic is self:
			return match.group(0)
		return '{}{}{}'.format(match.group(1), symbolic, match.group(3))

def main():
	import re, sys
	argv = sys.argv
	if len(argv) != 2:
		print('usage: {} </path/to/key.h>'.format(argv[0]), file=sys.stderr)
		sys.exit(1)
	match = re.compile(r'#define\s+(KEY\w+)\s+(0x[0-9a-fA-F]{1,2})$').match
	reverse_cpp_mapping = ReverseMapping()
	with open(argv[1], 'rt') as key_header:
		for line in key_header:
			line = line.strip()
			m = match(line)
			if not m:
				continue
			i = int(m.group(2), 16)
			reverse_cpp_mapping[i] = m.group(1)
	sub = re.compile(r'(\s*)\b(0x[0-9a-fA-F]{1,2})\b(\s*)').sub
	for line in sys.stdin:
		print(','.join([sub(reverse_cpp_mapping.re_sub, field) for field in line.rstrip().split(',')]))

if __name__ == '__main__':
	main()

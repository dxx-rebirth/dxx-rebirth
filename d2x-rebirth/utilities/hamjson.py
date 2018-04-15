#!/usr/bin/python3

import collections, json, os, struct

# Various constants from the game engine.  These must be kept
# synchronized with the game.
NDL = 5
MAX_GUNS = 8
for defines in [
#common/main/robot.h
"""
#define N_ANIM_STATES   5
#define MAX_SUBMODELS 10
#define MAX_ROBOT_TYPES 85      // maximum number of robot types
#define MAX_ROBOT_JOINTS 1600
""",
#d2x-rebirth/main/weapon.h
"""
#define MAX_WEAPON_TYPES            70
""",
#common/main/polyobj.h
"""
#define MAX_POLYGON_MODELS 200
""",
#d2x-rebirth/main/bm.h
"""
#define MAX_OBJ_BITMAPS     610
""",
#d2x-rebirth/main/bm.c
"""
#define N_D2_ROBOT_TYPES		66
#define N_D2_ROBOT_JOINTS		1145
#define N_D2_POLYGON_MODELS     166
#define N_D2_OBJBITMAPS			422
#define N_D2_OBJBITMAPPTRS		502
#define N_D2_WEAPON_TYPES		62
""",
]:
	for define in defines.strip().split('\n'):
		(_, name, value, *_) = define.split()
		globals()[name] = int(value)

class SerializeableSingleType:
	class Instance:
		def __init__(self,descriptor,value):
			self.Struct = descriptor
			self.value = value
		def stwritephys(self,fp,**kwargs):
			s = self.Struct
			fp.write(s.pack(self.value))
	def streadphys(self,fp,**kwargs):
		s = self.Struct
		return s.unpack_from(fp.read(s.size))
	def streadjs(self,js,name,**kwargs):
		assert(name)
		return js

class FixedIntegerArrayType(struct.Struct):
	class Instance:
		def __init__(self,array_descriptor,value):
			element_descriptor = array_descriptor.element_descriptor
			self.value = [element_descriptor.Instance(element_descriptor,v) for v in value]
		def stwritephys(self,fp):
			for value in self.value:
				value.stwritephys(fp=fp)
	def __init__(self,element_descriptor,element_count):
		self.element_descriptor = element_descriptor
		struct.Struct.__init__(self,b'<' + (element_descriptor.format[1:] * element_count))
		self.Struct = self
	def streadphys(self,fp,**kwargs):
		return self.Instance(self,SerializeableSingleType.streadphys(self,fp))
	def streadjs(self,js,name,**kwargs):
		return self.Instance(self,SerializeableSingleType.streadjs(self,js,name))

class IntegerType(struct.Struct):
	class Instance(SerializeableSingleType.Instance):
		def __init__(self,descriptor,value):
			SerializeableSingleType.Instance.__init__(self,descriptor,int(value))
	def __init__(self,format_string):
		struct.Struct.__init__(self,format_string)
		self.Struct = self
	def __mul__(self,length):
		return type('array%u.%s' % (length, self.__class__.__name__), (FixedIntegerArrayType,), {})(self, length)
	def streadphys(self,fp,**kwargs):
		value = SerializeableSingleType.streadphys(self,fp)
		assert(len(value) == 1)
		return self.Instance(self,value[0])
	def streadjs(self,js,name,**kwargs):
		return self.Instance(self,SerializeableSingleType.streadjs(self,js,name))
	@classmethod
	def create(cls,name,format_string):
		return type(name, (cls,), {})(format_string)

int16 = IntegerType.create('int16', '<h')
int32 = IntegerType.create('int32', '<i')
uint8 = IntegerType.create('uint8', '<B')
uint32 = IntegerType.create('uint32', '<I')

class FixedStructArrayType:
	class Instance:
		def __init__(self,value):
			self.value = value
		def stwritephys(self,fp):
			for value in self.value:
				value.stwritephys(fp=fp)
	def __init__(self,element_descriptor,element_count):
		self.element_descriptor = element_descriptor
		self.element_count = element_count
	def streadphys(self,fp,**kwargs):
		return self.Instance([self.element_descriptor.streadphys(fp=fp) for i in range(self.element_count)])
	def streadjs(self,js,**kwargs):
		return self.Instance([self.element_descriptor.streadjs(js=js[i]) for i in range(self.element_count)])

class SerializeableStructType:
	class Instance:
		def __init__(self,owner):
			# Ordering is not required for correct operation, but it makes
			# debugging output easier to read.
			self.value = collections.OrderedDict()
			self.__owner = owner
		def stwritephys(self,fp):
			for (field_name,field_type) in self.__owner._struct_fields_:
				if field_name is None:
					field_type.stwritephys(fp=fp,container=self)
				else:
					self.value[field_name].stwritephys(fp=fp)
	class MissingMemberError(KeyError):
		pass
	@classmethod
	def create(cls,name,fields):
		t = type(name, (cls,), {})
		t._struct_fields_ = fields
		return t()
	def streadphys(self,fp,**kwargs):
		result = self.Instance(self)
		for (name,field_type) in self._struct_fields_:
			value = field_type.streadphys(fp=fp, container=result)
			if name:
				result.value[name] = value
		return result
	def streadjs(self,js,**kwargs):
		result = self.Instance(self)
		for (field_name,field_type) in self._struct_fields_:
			field_value = js
			if field_name:
				if not field_name in js:
					raise self.MissingMemberError(field_name)
				field_value = js[field_name]
			value = field_type.streadjs(js=field_value, container=result, name=field_name)
			if field_name:
				result.value[field_name] = value
		return result
	def __mul__(self,length):
		return type('array%u.%s' % (length, self.__class__.__name__), (FixedStructArrayType,), {})(self, length)

class MagicNumberType(IntegerType):
	class Instance(IntegerType.Instance):
		def __init__(self,descriptor,value):
			IntegerType.Instance.__init__(self,descriptor,value)
			descriptor.check_magic(self)
	class MagicNumberError(ValueError):
		def __init__(self,value,expected_number):
			ValueError.__init__(self, "Invalid magic number: wanted %.8x, got %.8x" % (expected_number, value))
	def __init__(self,expected_number,format_string = uint32.format):
		IntegerType.__init__(self, format_string)
		self.expected_number = expected_number
	def check_magic(self,value):
		if value.value != self.expected_number:
			raise self.MagicNumberError(value.value, self.expected_number)

class VariadicArray:
	class MismatchLengthError(ValueError):
		pass
	class MinimumLengthError(ValueError):
		pass
	class MaximumLengthError(ValueError):
		pass
	def __init__(self,field,minimum=0,maximum=None,format_struct=uint32):
		self.field = field
		self.minimum = minimum
		self.maximum = maximum
		self.Struct = format_struct
	def streadphys(self,fp,container,**kwargs):
		element_count = SerializeableSingleType.streadphys(self.Struct, fp)[0]
		if element_count < self.minimum:
			raise MinimumLengthError("Invalid element count, must be at least %u, got %.8x; fp=%.8x" % (self.minimum, element_count, fp.tell()))
		if self.maximum is not None and element_count >= self.maximum:
			raise MaximumLengthError("Invalid element count, must be less than %u, got %.8x; fp=%.8x" % (self.maximum, element_count, fp.tell()))
		for (field_name,field_type) in self.field:
			streadphys = field_type.streadphys
			container.value[field_name] = [streadphys(fp=fp,container=container,i=i) for i in range(element_count)]
	def streadjs(self,js,container,name,**kwargs):
		assert(name is None)
		element_count = None
		for (field_name,field_type) in self.field:
			value = js[field_name]
			lvalue = len(value)
			if element_count is not None and element_count != lvalue:
				raise MismatchLengthError("Invalid element count, must be %u to match prior list, but got %u" % (element_count, lvalue))
			element_count = lvalue
			streadjs = field_type.streadjs
			container.value[field_name] = [streadjs(js=value[i],container=container,name=field_name) for i in range(element_count)]
		if element_count is None:
			if self.minimum > 0:
				raise MinimumLengthError("Invalid element count, must be at least %u, got None" % (self.minimum))
		else:
			if element_count < self.minimum:
				raise MinimumLengthError("Invalid element count, must be at least %u, got %.8x" % (self.minimum, element_count))
			if self.maximum is not None and element_count >= self.maximum:
				raise MaximumLengthError("Invalid element count, must be less than %u, got %.8x" % (self.maximum, element_count))
	def stwritephys(self,fp,container):
		element_count = None
		for (field_name,field_type) in self.field:
			value = container.value[field_name]
			lvalue = len(value)
			if element_count is not None and element_count != lvalue:
				raise MismatchLengthError("Invalid element count, must be %u to match prior list, but got %u" % (element_count, lvalue))
			element_count = lvalue
		if element_count is None:
			element_count = 0
		fp.write(self.Struct.pack(element_count))
		for (field_name,field_type) in self.field:
			for value in container.value[field_name]:
				value.stwritephys(fp=fp)

class ByteBlobType:
	class Instance:
		def __init__(self,value):
			self.__value = value
		def stwritephys(self,fp):
			fp.write(self.__value)
		@property
		def value(self):
			return tuple(self.__value)
	def streadjs(self,js,**kwargs):
		return self.Instance(bytes(js))

class D2PolygonData(ByteBlobType):
	def streadphys(self,fp,i,container,**kwargs):
		size = container.value['polygon_models'][i].value[D2PolygonModel.model_data_size].value
		return self.Instance(fp.read(size))

class TailPaddingType(ByteBlobType):
	def streadphys(self,fp,**kwargs):
		return self.Instance(fp.read())

class PHYSFSX:
	Byte = uint8
	Short = FixAng = int16
	Int = Fix = int32
	Vector = SerializeableStructType.create('PHYSFSX.Vector', (
		('x', int32),
		('y', int32),
		('z', int32),
		))
	AngleVec = SerializeableStructType.create('PHYSFSX.AngleVec', (
		('p', int16),
		('b', int16),
		('h', int16),
		))

BitmapIndex = SerializeableStructType.create('BitmapIndex', (
	('index', PHYSFSX.Short),
	))

WeaponInfo = SerializeableStructType.create('WeaponInfo', (
	('render_type', PHYSFSX.Byte),
	('persistent', PHYSFSX.Byte),
	('model_num', PHYSFSX.Short),
	('model_num_inner', PHYSFSX.Short),
	('flash_vclip', PHYSFSX.Byte),
	('robot_hit_vclip', PHYSFSX.Byte),
	('flash_sound', PHYSFSX.Short),
	('wall_hit_vclip', PHYSFSX.Byte),
	('fire_count', PHYSFSX.Byte),
	('robot_hit_sound', PHYSFSX.Short),
	('ammo_usage', PHYSFSX.Byte),
	('weapon_vclip', PHYSFSX.Byte),
	('wall_hit_sound', PHYSFSX.Short),
	('destroyable', PHYSFSX.Byte),
	('matter', PHYSFSX.Byte),
	('bounce', PHYSFSX.Byte),
	('homing_flag', PHYSFSX.Byte),
	('speedvar', PHYSFSX.Byte),
	('flags', PHYSFSX.Byte),
	('flash', PHYSFSX.Byte),
	('afterburner_size', PHYSFSX.Byte),
	('children', PHYSFSX.Byte),
	('energy_usage', PHYSFSX.Fix),
	('fire_wait', PHYSFSX.Fix),
	('multi_damage_scale', PHYSFSX.Fix),
	('bitmap', BitmapIndex),
	('blob_size', PHYSFSX.Fix),
	('flash_size', PHYSFSX.Fix),
	('impact_size', PHYSFSX.Fix),
	('strength', PHYSFSX.Fix * NDL),
	('speed', PHYSFSX.Fix * NDL),
	('mass', PHYSFSX.Fix),
	('drag', PHYSFSX.Fix),
	('thrust', PHYSFSX.Fix),
	('po_len_to_width_ratio', PHYSFSX.Fix),
	('light', PHYSFSX.Fix),
	('lifetime', PHYSFSX.Fix),
	('damage_radius', PHYSFSX.Fix),
	('picture', BitmapIndex),
	('hires_picture', BitmapIndex),
	))

Joint = SerializeableStructType.create('Joint', (
	('n_joints', PHYSFSX.Short),
	('offset', PHYSFSX.Short),
	))

AnimationState = SerializeableStructType.create('AnimationState', (
	('joints', Joint * N_ANIM_STATES),
	))

D2RobotInfo = SerializeableStructType.create('D2RobotInfo', (
	('model_num', PHYSFSX.Int),
	('gun_points', PHYSFSX.Vector * MAX_GUNS),
	('gun_submodels', PHYSFSX.Byte * MAX_GUNS),
	('exp1_vclip_num', PHYSFSX.Short),
	('exp1_sound_num', PHYSFSX.Short),
	('exp2_vclip_num', PHYSFSX.Short),
	('exp2_sound_num', PHYSFSX.Short),
	('weapon_type', PHYSFSX.Byte),
	('weapon_type2', PHYSFSX.Byte),
	('n_guns', PHYSFSX.Byte),
	('contains_id', PHYSFSX.Byte),
	('contains_count', PHYSFSX.Byte),
	('contains_prob', PHYSFSX.Byte),
	('contains_type', PHYSFSX.Byte),
	('kamikaze', PHYSFSX.Byte),
	('score_value', PHYSFSX.Short),
	('badass', PHYSFSX.Byte),
	('energy_drain', PHYSFSX.Byte),
	('lighting', PHYSFSX.Fix),
	('strength', PHYSFSX.Fix),
	('mass', PHYSFSX.Fix),
	('drag', PHYSFSX.Fix),
	('field_of_view', PHYSFSX.Fix * NDL),
	('firing_wait', PHYSFSX.Fix * NDL),
	('firing_wait2', PHYSFSX.Fix * NDL),
	('turn_time', PHYSFSX.Fix * NDL),
	('max_speed', PHYSFSX.Fix * NDL),
	('circle_distance', PHYSFSX.Fix * NDL),
	('rapidfire_count', PHYSFSX.Byte * NDL),
	('evade_speed', PHYSFSX.Byte * NDL),
	('cloak_type', PHYSFSX.Byte),
	('attack_type', PHYSFSX.Byte),
	('see_sound', PHYSFSX.Byte),
	('attack_sound', PHYSFSX.Byte),
	('claw_sound', PHYSFSX.Byte),
	('taunt_sound', PHYSFSX.Byte),
	('boss_flag', PHYSFSX.Byte),
	('companion', PHYSFSX.Byte),
	('smart_blobs', PHYSFSX.Byte),
	('energy_blobs', PHYSFSX.Byte),
	('thief', PHYSFSX.Byte),
	('pursuit', PHYSFSX.Byte),
	('lightcast', PHYSFSX.Byte),
	('death_roll', PHYSFSX.Byte),
	('flags', PHYSFSX.Byte),
	('pad', PHYSFSX.Byte * 3),
	('deathroll_sound', PHYSFSX.Byte),
	('glow', PHYSFSX.Byte),
	('behavior', PHYSFSX.Byte),
	('aim', PHYSFSX.Byte),
	('anim_states', AnimationState * (MAX_GUNS + 1)),
	('always_0xabcd', MagicNumberType(0xabcd)),
	))

D2RobotJoints = SerializeableStructType.create('D2RobotJoints', (
	('jointnum', PHYSFSX.Short),
	('angles', PHYSFSX.AngleVec),
	))

D2PolygonModel = SerializeableStructType.create('D2PolygonModel', (
	('n_models', PHYSFSX.Int),
	('model_data_size', PHYSFSX.Int),
	('model_data_ptr', PHYSFSX.Int),
	('submodel_ptrs', PHYSFSX.Int * MAX_SUBMODELS),
	('submodel_offsets', PHYSFSX.Vector * MAX_SUBMODELS),
	('submodel_norms', PHYSFSX.Vector * MAX_SUBMODELS),
	('submodel_pnts', PHYSFSX.Vector * MAX_SUBMODELS),
	('submodel_rads', PHYSFSX.Fix * MAX_SUBMODELS),
	('submodel_parents', uint8 * MAX_SUBMODELS),
	('submodel_mins', PHYSFSX.Vector * MAX_SUBMODELS),
	('submodel_maxs', PHYSFSX.Vector * MAX_SUBMODELS),
	('mins', PHYSFSX.Vector),
	('maxs', PHYSFSX.Vector),
	('rad', PHYSFSX.Fix),
	('n_textures', PHYSFSX.Byte),
	('first_texture', PHYSFSX.Short),
	('simpler_model', PHYSFSX.Byte),
	))

D2PolygonModel.model_data_size = 'model_data_size'

HAM1 = SerializeableStructType.create('HAM1', (
	('signature', MagicNumberType(1481130317)),	# 'XHAM'
	('version', uint32),
	(None, VariadicArray((('weapon_info', WeaponInfo),), 0, MAX_WEAPON_TYPES - N_D2_WEAPON_TYPES)),
	(None, VariadicArray((('robot_info', D2RobotInfo),), 0, MAX_ROBOT_TYPES - N_D2_ROBOT_TYPES)),
	(None, VariadicArray((('robot_joints', D2RobotJoints),), 0, MAX_ROBOT_JOINTS - N_D2_ROBOT_JOINTS)),
	(None, VariadicArray((
		('polygon_models', D2PolygonModel),
		('polymodel_data', D2PolygonData()),
		('dying_modelnum', int32),
		('dead_modelnum', int32),
	), 0, MAX_POLYGON_MODELS - N_D2_POLYGON_MODELS)),
	(None, VariadicArray((
		('obj_bitmaps', BitmapIndex),
	), 0, MAX_OBJ_BITMAPS - N_D2_OBJBITMAPS)),
	(None, VariadicArray((
		('obj_bitmap_ptrs', int16),
	), 0, MAX_OBJ_BITMAPS - N_D2_OBJBITMAPPTRS)),
	('tail_padding', TailPaddingType())
	))

class D2HXM1(SerializeableStructType):
	RobotTypeIndex = uint32
	RobotJointIndex = uint32
	PolygonIndex = uint32
	ObjBitmapIndex = uint32
	class D2HXMPolygonDataType(ByteBlobType):
		polygon_model = 'polygon_model'
		def streadphys(self,fp,container):
			size = container.value[self.polygon_model].value[D2PolygonModel.model_data_size].value
			return self.Instance(fp.read(size))
	_struct_fields_ = (
		('signature', MagicNumberType(0x21584d48)),	# '!MXH'
		('version', uint32),
		(None, VariadicArray((
			('replace_robot', SerializeableStructType.create('ReplaceRobot', (
				('index', RobotTypeIndex),
				('robot_info', D2RobotInfo),
			))),
		))),
		(None, VariadicArray((
			('replace_joint', SerializeableStructType.create('ReplaceJoint', (
				('index', RobotJointIndex),
				('robot_joints', D2RobotJoints),
			))),
		))),
		(None, VariadicArray((
			('replace_polygon', SerializeableStructType.create('ReplacePolygon', (
				('index', PolygonIndex),
				(D2HXMPolygonDataType.polygon_model, D2PolygonModel),
				('polymodel_data', D2HXMPolygonDataType()),
				('dying_modelnum', int32),
				('dead_modelnum', int32),
			))),
		))),
		(None, VariadicArray((
			('replace_objbitmap', SerializeableStructType.create('ReplaceObjBitmap', (
				('index', ObjBitmapIndex),
				('obj_bitmap', BitmapIndex),
			))),
		))),
		(None, VariadicArray((
			('replace_objbitmapptr', SerializeableStructType.create('', (
				('index', ObjBitmapIndex),
				('obj_bitmap_ptr', int16),
			))),
		))),
		('tail_padding', TailPaddingType())
	)

HXM1 = D2HXM1()

class JSONEncoder(json.JSONEncoder):
	def default(self,o):
		try:
			return o.value
		except AttributeError:
			pass
		return json.JSONEncoder.default(self, o)

class FileFormat:
	ExtensionTypeMap = {
		'ham': HAM1,
		'hxm': HXM1,
	}
	class UnknownFormatError(KeyError):
		pass
	class InvalidFormatError(KeyError):
		pass
	@classmethod
	def guess(cls,infile,outfile,arg):
		for filename in (infile, outfile):
			(b, ext) = os.path.splitext(filename)
			ext = ext.lower()
			if ext == '.json':
				ext = os.path.splitext(b)[1].lower()
			result = cls.ExtensionTypeMap.get(ext.lstrip('.'))
			if result:
				return result
		raise cls.UnknownFormatError("Unknown file format specify format using --%s" % arg)
	@classmethod
	def get(cls,infile,fmt,outfile,arg):
		inJSON = (os.path.splitext(infile)[1].lower().lstrip('.') == 'json')
		outJSON = (os.path.splitext(outfile)[1].lower().lstrip('.') == 'json')
		ecls = cls.ExtensionTypeMap[fmt] if fmt else FileFormat.guess(infile, outfile, arg)
		encodeToJSON = outJSON if inJSON ^ outJSON else None
		return (ecls, encodeToJSON)

def main():
	class IndentCheck:
		choices = ['tab', 'space', 'none']
		class Iterator:
			def __init__(self,choices):
				self.index = 0
				self.choices = choices
			def __iter__(self):
				return self
			def __next__(self):
				i = self.index
				self.index = i + 1
				if i < len(self.choices):
					return self.choices[i]
				raise StopIteration
		def __contains__(self,value):
			return value.strip() == '' or value in self.choices
		def __iter__(self):
			return self.Iterator(self.choices)
	import argparse
	parser = argparse.ArgumentParser(description='convert Descent data files to/from binary/JSON')
	parser.add_argument('--format', choices=FileFormat.ExtensionTypeMap.keys(), help='binary file format to use', metavar='FORMAT')
	parser.add_argument('--indent', choices=IndentCheck(), help='how to indent JSON output', default='\t')
	group = parser.add_mutually_exclusive_group()
	group.add_argument('--encode', action='store_true', default=None, help='treat input as binary and output as JSON')
	group.add_argument('--decode', dest='encode', action='store_false', help='treat input as JSON and output as binary')
	parser.add_argument('input', help='file to read', metavar='input-file')
	parser.add_argument('output', help='file to write', metavar='output-file')
	args = parser.parse_args()
	(cls, encodeToJSON) = FileFormat.get(args.input,args.format,args.output,'format')
	if args.encode is not None:
		encodeToJSON = args.encode
	if encodeToJSON is None:
		parser.error("Neither input nor output ends in json.  Use --encode or --decode to specify whether to produce JSON or consume JSON.")
	if encodeToJSON:
		with open(args.input, 'rb') as f:
			i = cls.streadphys(f)
		indent = args.indent
		if indent == 'space':
			indent = ' '
		elif indent == 'tab':
			indent = '\t'
		elif indent == 'none':
			indent = ''
		with open(args.output, 'wt') as f:
			json.dump(i, f, cls=JSONEncoder, indent=indent)
	else:
		with open(args.input, 'rt') as f:
			i = cls.streadjs(js=json.load(f, object_pairs_hook=collections.OrderedDict))
		with open(args.output, 'wb') as f:
			i.stwritephys(f)

if __name__ == '__main__':
	main()

import ninja_syntax
import os

b = """
                                 _        _       
 ___  ___ ___  _ __  ___   _ __ (_)_ __  (_) __ _ 
/ __|/ __/ _ \| '_ \/ __| | '_ \| | '_ \ | |/ _` |
\__ \ (_| (_) | | | \__ \ | | | | | | | || | (_| |
|___/\___\___/|_| |_|___/ |_| |_|_|_| |_|/ |\__,_|
                                       |__/       

"""

def expand_include(include):

	new_include = []

	for i in include:
		s =  i.replace('#','./')
		s = '-I' + s
		new_include.append(s)

	return new_include


def expand_libs(libs):

	new_libs = []

	for l in libs:
		if type(l) == str:
			new_libs.append( "-l" + l)
		else:
			for n in l:
				new_libs.append( n.rstr() )

	return new_libs


def banner(ninja):
	for line in b.split('\n'):
		ninja.comment(line)

	ninja.newline()


def variables(ninja, env):

	ninja.variable('CPPFLAGS', env['CPPFLAGS'])
	ninja.newline()

	ninja.variable('CFLAGS', env['CFLAGS'])
	ninja.newline()

	ninja.variable('CCFLAGS', env['CCFLAGS'])
	ninja.newline()

	ninja.variable('CXXFLAGS', env['CXXFLAGS'])
	ninja.newline()

	ninja.variable('CPPPATH', expand_include( env['CPPPATH'] ))
	ninja.newline()

	ninja.variable('LIBS', expand_libs( env['LIBS'] ))
	ninja.newline()


def rules(ninja, env):
	ninja.rule(	name		=	'CC',
			command		=	env['CC'] + ' $CFLAGS $CCFLAGS $CXXFLAGS $CPPFLAGS $CPPPATH -o $out -c $in' ,
			description	=	'Building CC object $out',
			depfile		=	'$DEP_FILE',
			deps		=	'gcc')
		
	ninja.newline()
	
	ninja.rule(	name		=	'CXX',
			command		=	env['CXX'] + ' $CCFLAGS $CXXFLAGS $CPPFLAGS $CPPPATH -o $out -c $in' ,
			description	=	'Building CXX object $out',
			depfile		=	'$DEP_FILE',
			deps		=	'gcc')

	ninja.newline()

	ninja.rule(	name		=	'STATIC_LIBRARY',
			command		=	'ar qc $out $in && ranlib $out',
			description	=	'Linking static library $out')
			
	ninja.newline()

	ninja.rule(	name		=	'CXX_PROG',
			command		=	env['CXX'] + ' -o $out $LIBS $in',
			description	=	'Linking program $out') 
	
	ninja.newline()

	ninja.rule(	name		=	'CLEAN',
  			command 	= 	'ninja -t clean',
  			description 	= 	'Cleaning all built files...')

	ninja.newline()


def end(env):
	print 'Ninja END'

	if env['ninja'] != None:

		ninja	= env['ninja_writer']

		#ninja.build(	outputs	=	'clean',
		#		rule	=	'CLEAN'		)

		ninja.close()


def begin(env):
	print 'ninja BEGIN'

	ninja = env['ninja_writer']
	banner(ninja)
	variables(ninja, env)
	rules(ninja, env)



#   ______  ____  __
#  / ___\ \/ /\ \/ /
# | |    \  /  \  / 
# | |___ /  \  /  \ 
#  \____/_/\_\/_/\_\
#                  
def action_obj_cxx(target, source, env):

	ninja_out 	= []
	ninja_in 	= []

	for t in target:
		#print "CXX Target..." +  str(t)
		ninja_out.append(str(t))
		ninja_var =( { 'depfile' : str(t) + '.d' } )

	for s in source:
		#print "CXX Source..." + str(s)
		ninja_in.append(str(s))

	ninja = env['ninja_writer']
	ninja.build(	outputs		=	ninja_out,
			rule		=	"CXX",
			inputs		=	ninja_in,
			variables	=	ninja_var ) 

	ninja.newline()

	return None


def action_obj_cxx_print(target, source, env):
	pass

#   ____ ____ 
#  / ___/ ___|
# | |  | |    
# | |__| |___ 
#  \____\____|
#    
def action_obj_cc(target, source, env):
	
	ninja_out = []
	ninja_in = []

	for t in target:
		#print "CC Target...", t
		ninja_out.append(str(t))
		ninja_var =( { 'depfile' : str(t) + '.d' } )

	for s in source:
		#print "CC Source...",s
		ninja_in.append(str(s))

	ninja = env['ninja_writer']
	ninja.build(	outputs		=	ninja_out,
			rule		=	"CC",
			inputs		=	ninja_in,
			variables	=	ninja_var ) 

	ninja.newline()

	return None


def action_obj_cc_print(target, source, env):
	pass


#  _     ___ ____  
# | |   |_ _| __ ) 
# | |    | ||  _ \ 
# | |___ | || |_) |
# |_____|___|____/ 
#                 
def action_lib(target, source, env):

	lib_obj = []
	lib = ''

	for t in target:
		print "Library Target...", t
		lib = str(t)


	for s in source:
		#print "Library Source...",s

		name, ext = os.path.splitext(str(s))

		if ext == '.cpp':
			target = []
			source = []

			target.append( name + '.o' )
			source.append( str(s) )

			action_obj_cxx( target, source, env)

			lib_obj.append( name + '.o' )

		if ext == '.c':
			target = []
			source = []

			target.append( name + '.o' )
			source.append( str(s) )

			action_obj_cc( target, source, env)

			lib_obj.append( name + '.o' )

		if ext == '.o':
			lib_obj.append( name + '.o' )

	ninja = env['ninja_writer']
	ninja.build(	outputs		=	lib,
			rule		=	'STATIC_LIBRARY',
			inputs		=	lib_obj ) 

	ninja.newline()

	return None

	
def action_lib_print(target, source, env):
	pass


#  ____  ____   ___   ____ ____      _    __  __ 
# |  _ \|  _ \ / _ \ / ___|  _ \    / \  |  \/  |
# | |_) | |_) | | | | |  _| |_) |  / _ \ | |\/| |
# |  __/|  _ <| |_| | |_| |  _ <  / ___ \| |  | |
# |_|   |_| \_\\___/ \____|_| \_\/_/   \_\_|  |_|
#                                               
def action_prog(target, source, env):

	prog_obj = []
	prog = ''

	for t in target:
		print "Program Target...", t
		prog = str(t)

	for s in source:
		print "Program Source...",s

		name, ext = os.path.splitext(str(s))
		
		if ext == '.cpp':
			target = []
			source = []

			target.append( name + '.o' )
			source.append( str(s) )

			action_obj_cxx( target, source, env)

			prog_obj.append( name + '.o' )

		if ext == '.c':
			target = []
			source = []

			target.append( name + '.o' )
			source.append( str(s) )

			action_obj_cc( target, source, env)

			prog_obj.append( name + '.o' )

		if ext == '.o':
			prog_obj.append( name + '.o' )

		print type(s)

	for l in env['LIBS']:
		print "Program Libs...", l, type(l)
		if type(l) != type(''):
			for a in l:
				print a, type(a)

	ninja = env['ninja_writer']
	ninja.build(	outputs		=	prog,
			rule		=	'CXX_PROG',
			inputs		=	prog_obj ) 

	ninja.newline()

	return None


def action_prog_print(target, source, env):
	pass

#  _____ _   ___     _____ ____   ___  _   _ __  __ _____ _   _ _____ 
# | ____| \ | \ \   / /_ _|  _ \ / _ \| \ | |  \/  | ____| \ | |_   _|
# |  _| |  \| |\ \ / / | || |_) | | | |  \| | |\/| |  _| |  \| | | |  
# | |___| |\  | \ V /  | ||  _ <| |_| | |\  | |  | | |___| |\  | | |  
# |_____|_| \_|  \_/  |___|_| \_\\___/|_| \_|_|  |_|_____|_| \_| |_|  
#                                                                    
def environment(env):

	from SCons.Builder import Builder

	import SCons.exitfuncs
	SCons.exitfuncs.register(end, env)

	f = open('build.ninja', 'w')

	env['ninja_writer'] = ninja_syntax.Writer(f)
	env['ninja_begin'] = begin
	env['ninja_end'] = end

	act_obj_cc = env.Action( action_obj_cc, strfunction=action_obj_cc_print)
	act_obj_cxx = env.Action( action_obj_cxx, strfunction=action_obj_cxx_print)
	act_lib = env.Action( action_lib, strfunction=action_lib_print)
	act_prog = env.Action( action_prog, strfunction=action_prog_print)

	builder_obj = Builder(	action = { 	'.c'	:	act_obj_cc,
						'.cpp'	:	act_obj_cxx },
				suffix = '.o'
				)

	builder_lib = Builder(	action = act_lib,
				suffix = '.a',
				)

	builder_prog = Builder(	action = act_prog,
				suffix = '',
				)

	return env.Clone(BUILDERS = {	'Object'	:	builder_obj,
					'Library'	:	builder_lib,
					'Program'	:	builder_prog	})

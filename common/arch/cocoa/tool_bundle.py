# Create an application package.

# https://www.scons.org/wiki/SubstInFileBuilder

import re
from SCons.Script import *
from SCons.Builder import *

def TOOL_SUBST(env):
    """Adds SubstInFile builder, which substitutes the keys->values of SUBST_DICT
    from the source to the target.
    The values of SUBST_DICT first have any construction variables expanded
    (its keys are not expanded).
    If a value of SUBST_DICT is a python callable function, it is called and
    the result is expanded as the value.
    If there's more than one source and more than one target, each target gets
    substituted from the corresponding source.
    """
    env.Append(TOOLS = 'SUBST')
    def do_subst_in_file(targetfile, sourcefile, dict):
        """Replace all instances of the keys of dict with their values.
        For example, if dict is {'%VERSION%': '1.2345', '%BASE%': 'MyProg'},
        then all instances of %VERSION% in the file will be replaced with 1.2345 etc.
        """
        try:
            f = open(sourcefile, 'rb')
            contents = f.read()
            f.close()
        except:
            raise SCons.Errors.UserError("Can't read source file %s" % (sourcefile,))
        for (k,v) in dict.items():
            try:
                contents = contents.decode("UTF-8")
            except (UnicodeDecodeError, AttributeError):
                pass
            contents = re.sub(k, v, contents)
        try:
            f = open(targetfile, 'wt')
            f.write(contents)
            f.close()
        except Exception as e:
            print("Text:", str(e))
            raise SCons.Errors.UserError("Can't write target file %s" % (targetfile,))
        return 0 # success

    def subst_in_file(target, source, env):
        if 'SUBST_DICT' not in env:
            raise SCons.Errors.UserError("SubstInFile requires SUBST_DICT to be set.")
        d = dict(env['SUBST_DICT']) # copy it
        for (k,v) in d.items():
            if callable(v):
                d[k] = env.subst(v())
            elif SCons.Util.is_String(v):
                d[k]=env.subst(v)
            else:
                raise SCons.Errors.UserError("SubstInFile: key %s: %r must be a string or callable" % (k, v))
        for (t,s) in zip(target, source):
            return do_subst_in_file(str(t), str(s), d)

    def subst_in_file_string(target, source, env):
        """This is what gets printed on the console."""
#        return '\n'.join(['Substituting vars from %s into %s'%(str(s), str(t))
#                          for (t,s) in zip(target, source)])

    def subst_emitter(target, source, env):
        """Add dependency from substituted SUBST_DICT to target.
        Returns original target, source tuple unchanged.
        """
        d = env['SUBST_DICT'].copy() # copy it
        for (k,v) in d.items():
            if callable(v):
                d[k] = env.subst(v())
            elif SCons.Util.is_String(v):
                d[k]=env.subst(v)
        env.Depends(target, SCons.Node.Python.Value(d))
        return target, source

    subst_action=SCons.Action.Action(subst_in_file, subst_in_file_string)
    env['BUILDERS']['SubstInFile'] = env.Builder(action=subst_action, emitter=subst_emitter)

# https://www.scons.org/wiki/MacOSX (modified to suit)

from SCons.Defaults import SharedCheck, ProgScan
from SCons.Script.SConscript import SConsEnvironment
import sys
import os
import SCons.Node 
import SCons.Util
import string

def TOOL_BUNDLE(env):
    """defines env.LinkBundle() for linking bundles on Darwin/OSX, and
       env.MakeBundle() for installing a bundle into its dir.
       A bundle has this structure: (filenames are case SENSITIVE)
       sapphire.bundle/
         Contents/
           Info.plist (an XML key->value database; defined by BUNDLE_INFO_PLIST)
           PkgInfo (trivially short; defined by value of BUNDLE_PKGINFO)
           MacOS/
             executable (the executable or shared lib, linked with Bundle())
    Resources/
         """
    if 'BUNDLE' in env['TOOLS']: return
    #if tools_verbose:
    print(" running tool: TOOL_BUNDLE")
    env.Append(TOOLS = 'BUNDLE')
    # This is like the regular linker, but uses different vars.
    # XXX: NOTE: this may be out of date now, scons 0.96.91 has some bundle linker stuff built in.
    # Check the docs before using this.
    """LinkBundle = env.Builder(action=[SharedCheck, "$BUNDLECOM"],
                                       emitter="$SHLIBEMITTER",
                                       prefix = '$BUNDLEPREFIX',
                                       suffix = '$BUNDLESUFFIX',
                                       target_scanner = ProgScan,
                                       src_suffix = '$BUNDLESUFFIX',
                                       src_builder = 'SharedObject')
    env['BUILDERS']['LinkBundle'] = LinkBundle"""
    env['BUNDLEEMITTER'] = None
    env['BUNDLEPREFIX'] = ''
    env['BUNDLESUFFIX'] = ''
    env['BUNDLEDIRSUFFIX'] = '.bundle'
    #env['FRAMEWORKS'] = ['-framework Carbon', '-framework System']
    env['BUNDLE'] = '$SHLINK'
    env['BUNDLEFLAGS'] = ' -bundle'
    env['BUNDLECOM'] = '$BUNDLE $BUNDLEFLAGS -o ${TARGET} $SOURCES $_LIBDIRFLAGS $_LIBFLAGS $FRAMEWORKS'
    # This requires some other tools:
    TOOL_SUBST(env)
    # Common type codes are BNDL for generic bundle and APPL for application.
    def MakeBundle(env, bundledir, app,
                   key, info_plist,
                   typecode='BNDL', creator='SapP',
                   icon_file='#macosx-install/sapphire-icon.icns',
                   subst_dict=None,
                   resources=[]):
        """Install a bundle into its dir, in the proper format"""
        # Substitute construction vars:
        for a in [bundledir, key, info_plist, icon_file, typecode, creator]:
            a = env.subst(a)
        if SCons.Util.is_List(app):
            app = app[0]
        if SCons.Util.is_String(app):
            app = env.subst(app)
            appbase = os.path.basename(app)
        else:
            appbase = os.path.basename(str(app))
        if not ('.' in bundledir):
            bundledir += '.$BUNDLEDIRSUFFIX'
        bundledir = env.subst(bundledir) # substitute again
        suffix=bundledir[bundledir.rfind('.'):]
        if (suffix=='.app' and typecode != 'APPL' or
            suffix!='.app' and typecode == 'APPL'):
            raise Error("MakeBundle: inconsistent dir suffix %s and type code %s: app bundles should end with .app and type code APPL." % (suffix, typecode))
        if subst_dict is None:
            subst_dict={'%SHORTVERSION%': '$VERSION_NUM',
                        '%LONGVERSION%': '$VERSION_NAME',
                        '%YEAR%': '$COMPILE_YEAR',
                        '%BUNDLE_EXECUTABLE%': appbase,
                        '%ICONFILE%': os.path.basename(icon_file),
                        '%CREATOR%': creator,
                        '%TYPE%': typecode,
                        '%BUNDLE_KEY%': key}
        bundledir = env.Dir(bundledir)
        appfile = env.Install(bundledir.Dir('Contents/MacOS'), app)
        f = env.SubstInFile(bundledir.File('Contents/Info.plist'), info_plist,
                        SUBST_DICT=subst_dict)
        env.Depends(f, env.Value(key + creator + typecode + env['VERSION_NUM'] + env['VERSION_NAME']))
        env.Textfile(target=bundledir.File('Contents/PkgInfo'),
                     source=env.Value(typecode+creator))
        resources.append(icon_file)
        resource_directory = bundledir.Dir('Contents/Resources')
        for r in resources:
            if SCons.Util.is_List(r):
                env.InstallAs(resource_directory.File(r[1]), r[0])
            else:
                env.Install(resource_directory, r)
        return bundledir, appfile
    # This is not a regular Builder; it's a wrapper function.
    # So just make it available as a method of Environment.
    SConsEnvironment.MakeBundle = MakeBundle

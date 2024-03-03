Gettext is used to extract strings marked for localization from source code.
Each phrase to be translated must be wrapped with _TR() in header and source files.

Do the following steps for localization.

1. Set -DENABLE_TRANSLATIONS=True in cmake command line arguments. 
If you use build scripts from source folder then it is already done unless you set enable_translations=0 explicitly in configuration script.

2. To add new locale/language edit translations.txt in the source root folder and add new language/locale to the list separated by comma,
for example: 

(in file translations.txt)
ru,de

3. (Excluding MSVC). To generate or update po files run in the build root folder:

console$ make update-po

4. Localizaion .po files for the locale will be generated 
and put in translations folder of each subproject. Re-run cmake (build script) at least once 
to apply changes.

5. Edit .po files using PoEdit or other editor.

6. Target .mo files will be auto-generated each time you build the project.

7. (Excluding MSVC). If you need to update .po files after adding new strings to sources, just repeat step 3, i.e. run:

console$ make update-po

If you build only with MSVC then steps 3 and 7 are not applicable because there are no Makefiles.
Instead, you should run manually scripts/python/update-po-files.py in each subproject folder.
See description of command line parameters for the script.

When a new subproject is added steps 3-6 must be repeated.
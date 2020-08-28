On Windows:
1- Copy the *content* of the "Data" folder in C:/pv57/pvbd/ExternalData/Plugins/Fespp/Testing/Data (where pvbd is the build directory of your paraview). Create the latter directory if it does not already exist.
2- Start paraview.exe
3- Load the Fespp plugin if not already done (menu "Tools" -> submenu "Manage Plugins")
4- Choose menu "Tools" -> submenu "Play Test" to start
5- Select the fesapiExample.xml (located in this folder) and click OK
6- No error at all should raise in "Output Messages" window but only the line "Test  "fesapiExample.xml" is finished. Success =  true"

1- Set your PARAVIEW_DATA_ROOT environment variable (https://kitware.github.io/paraview-docs/v5.7.0/cxx/EnvironmentVariables.html)
2- Copy the *content* of the "Data" folder in PARAVIEW_DATA_ROOT/ExternalData/Plugins/Fespp/Testing/Data. Create the latter directory if it does not already exist.
3- Start paraview
4- Load the Fespp plugin if not already done (menu "Tools" -> submenu "Manage Plugins")
5- Enlarge the paraview window at a maximum on your screen
6- Choose menu "Tools" -> submenu "Play Test" to start
7- Select the fesapiExample.xml (located in this folder) and click OK
8- No error at all should raise in "Output Messages" window but only the line "Test  "fesapiExample.xml" is finished. Success =  true"

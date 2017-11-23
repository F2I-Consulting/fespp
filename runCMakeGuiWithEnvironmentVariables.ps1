##
#
# Example of a shell script to run cmake-gui with all the needed environement variables.
#
##

# Please modify the path to FESAPI_DIR according to your configuration
if ($Env:FESAPI_DIR)
{
	'FESAPI_DIR already set to ' + $Env:FESAPI_DIR
}
else
{
	'Need to set FESAPI_DIR'
	$Env:FESAPI_DIR = "D:\FESAPI\v0_9\install"
}


# Please modify the path to ParaView_DIR according to your configuration
if ($Env:ParaView_DIR)
{
	'ParaView_DIR already set to ' + $Env:ParaView_DIR
}
else
{
	'Need to set ParaView_DIR'
	$Env:ParaView_DIR = "D:\ParaView\paraview-build\PV_5_2"
}

# Please modify the path to cmake-gui according to your configuration
cmake-gui .

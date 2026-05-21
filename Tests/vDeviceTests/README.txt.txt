To execute the cases inside qemu

cd /tmp

git clone git@github.com:rdkcentral/entservices-inputoutput.git

cd entservices-inputoutput/vDeviceTests

python3 suiteManager.py Hdmicecsource # to execute hdmicecsource testcases


python3 suiteManager.py ledindicator 


errors incase if any:-

hdmicec_post_command.sh not found - 
here in the testcases add the path where hdmicec_post_command.sh script is present inside the qemu 
for eg tmp/vDeviceTests/vHdmiCec/vComponent_configurations/hdmicec_post_command.sh
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>

std::string Query();
std::string RandMac();
bool IsMAC(std::string& mac);

int main(int argc, char** argv)
{
	std::string NewMac = "";
	bool reset = false;
	if(argc == 2 || argc == 3)
	{
		std::string arg = std::string(argv[1]);
		if(arg == "-h" || arg == "--help")
		{
			printf("-h   --help\tDisplay this help dialogue\n\
-m   --mac\tSpecify MAC address instead of a random one\n\
-r   --reset\tReset to the default MAC address\n");
			return -1;
		}
		else if(arg == "-r" || arg == "--reset")
		{
			reset = true;
		}
		else if(arg == "-m" || arg == "--mac")
		{
			if(argc == 3)
			{
				NewMac = std::string(argv[2]);
				if(!IsMAC(NewMac))
				{
					printf("Not a proper MAC address, exiting\n");
					return -2;
				}
			}
			else
			{
				printf("Didn't understand the arguments, exiting\n");
				return -2;
			}
		}
		else 
		{
			printf("Didn't understand the arguments, exiting\n");
			return -2;
		}
	}
	else
		NewMac = RandMac();

	int err = 0;
	printf("Enter Network Adapter Name<Local Area Connection>: ");
	char c;
	std::string NAName = "";
	while(true)
	{
		scanf("%c", &c);
		if(c == '\n')
			break;
		else
			NAName += c;
	}
	if(NAName.size() == 0)
		NAName = "Wireless Network Connection";
	
	err = system(std::string(std::string("netsh interface set interface \"") + NAName + std::string("\" DISABLED")).c_str());
	if(err != 0)
	{
		scanf("%c", &c);
		return -3;
	}
	
	std::string KeyLoc = Query();
	if(KeyLoc == "***DUN_GOOFED***")
	{
		printf("Bad Entry, exiting\n");
		system(std::string(std::string("netsh interface set interface \"") + NAName + std::string("\" ENABLED")).c_str());
		scanf("%c", &c);
		return -4;
	}
	
	HKEY NICKey;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyLoc.c_str(), 0, KEY_WRITE, &NICKey);
	if(reset)
		RegSetValueEx(NICKey, "NetworkAddress", 0, REG_SZ, (const BYTE*)std::string("\0").c_str(), 1);
	else
		RegSetValueEx(NICKey, "NetworkAddress", 0, REG_SZ, (const BYTE*)NewMac.c_str(), 13);
	
	RegCloseKey(NICKey);
	system(std::string(std::string("netsh interface set interface \"") + NAName + std::string("\" ENABLED")).c_str());
	printf("Finished\n");
	scanf("%c", &c);
	return 0;
}

std::string RandMac()
{
	char WA[4] = {'2', '6', 'A', 'E'};
	srand(time(NULL));
	char AddrBuf[3];
	std::string NewMac = "";
	
	//Because of fucking wireless adapters...
	unsigned int MacP = rand() % 16;
	sprintf(AddrBuf, "%X:", MacP);
	NewMac += AddrBuf[0];
	MacP = rand() % 4;
	NewMac += WA[MacP];
	
	for(int i = 0; i < 5; i++)
	{
		MacP = rand() % 256;
		sprintf(AddrBuf, "%02X:", MacP);
		NewMac += AddrBuf[0];
		NewMac += AddrBuf[1];
	}
	printf("New MAC Address is: %s\n", NewMac.c_str());
	return NewMac;
}

bool IsMAC(std::string& mac)
{
	std::string testMac;
	for(int i = 0; i < mac.size(); i++)
	{
		//if	  Is not a decimal number			and		  Is not an acceptable upper case letter
		if(!((int)mac[i] >= 48 && (int)mac[i] < 58) && !((int)mac[i] >= 65 && (int)mac[i] < 71))
		{
			if(mac[i] == ':')
				continue;
			else if((int)mac[i] >= 97 && (int)mac[i] < 103)
				testMac.push_back(mac[i] - (char)32);
			else
				return false;
		}
		else
			testMac.push_back(mac[i]);
	}
	mac = testMac;
	return true;
}

std::string Query()
{
	std::string RtrnStr;

	HKEY MKey;
	long unsigned int SubKeysN = 0;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}", 0, KEY_READ, &MKey);
	RegQueryInfoKey(MKey, 0, 0, 0, &SubKeysN, 0, 0, 0, 0, 0, 0, 0);
	if(SubKeysN)
	{
		std::string* KeyNames = new std::string[SubKeysN];
		TCHAR NameBuf[255];
		long unsigned int NameLen;
		for(int i = 0; i < SubKeysN; i++)
		{
			NameLen = 255;
			memset(NameBuf, 0, 255*sizeof(TCHAR));
			if(RegEnumKeyEx(MKey, i, NameBuf, &NameLen, 0, 0, 0, 0) == 0)
			{
				for(int j = 0; j < NameLen; j++)
					KeyNames[i] += NameBuf[j];
			}
		}
		
		HKEY SubKey;
		char DescBuf[1024];
		long unsigned int DescSize;
		printf("Key Name        Driver Description\n-----------------------------------\n");
		for(int i = 0; i < SubKeysN; i++)
		{
			DescSize = 1024*sizeof(TCHAR);
			memset(DescBuf, 0, DescSize);
			RegOpenKeyEx(HKEY_LOCAL_MACHINE, std::string(std::string("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\") + KeyNames[i]).c_str(), 0, KEY_READ, &SubKey);
			long unsigned int err = RegQueryValueEx(SubKey, "DriverDesc\0", 0, 0, (unsigned char*)DescBuf, &DescSize);
			if(err == 0)
			{
				printf("%s\t\t%s\n", KeyNames[i].c_str(), std::string(DescBuf).c_str());
			}
			RegCloseKey(SubKey);
		}
		printf("\nEnter The Key Name Of The NIC To MAC Spoof: ");
		std::string NICKeyName;
		char c;
		while(true)
		{
			scanf("%c", &c);
			if(c == '\n')
				break;
			else
				NICKeyName += c;
		}
		if(NICKeyName.size() == 0)
			NICKeyName = "0007";
		
		bool BadName = true;
		for(int i = 0; i < SubKeysN; i++)
		{
			if(NICKeyName == KeyNames[i])
			{
				BadName = false;
				break;
			}
		}
		if(BadName)
			RtrnStr = "***DUN_GOOFED***";
		else
			RtrnStr = std::string("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\") + NICKeyName;
	}
	
	return RtrnStr;
}
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <fstream>

typedef struct _TOKEN_ELEVATION {
  DWORD TokenIsElevated;
} TOKEN_ELEVATION, *PTOKEN_ELEVATION;

std::string Query(std::string DevDesc);
std::string RandMac();
bool IsMAC(std::string& mac);
bool IsElevated();

int main(int argc, char** argv)
{
	if(!IsElevated())
	{
		printf("Not run as admin, exiting\n");
		return -9;
	}

	std::string NewMac = "";
	bool reset = false;
	
	//Figure out what they want done through args...
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
				printf("No MAC address given, exiting\n");
				return -3;
			}
		}
		else 
		{
			printf("Didn't understand the arguments, exiting\n");
			return -4;
		}
	}
	else
		NewMac = RandMac();
	
	if(!reset)
		printf("MAC Address will be set to %s\n", NewMac.c_str());

	//What adapter/connection does the user want to work with? (Default is "Wireless Network Connection")
	printf("Network Adapter Name <Eg. \"Wireless Network Connection\">: ");
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
	
	//Get list of all adapters
	system("ipconfig/all > adapters.list");
	
	std::string KeyLoc;
	std::fstream AdaptersList;
	
	//Scan list to obtain device description that matches given connection name
	AdaptersList.open("adapters.list", std::fstream::in);
	if(AdaptersList.is_open())
	{
		AdaptersList.seekg(0, AdaptersList.end);
		unsigned int FileSize = AdaptersList.tellg();
		AdaptersList.seekg(0, AdaptersList.beg);
		
		char* FileBuf = new char[FileSize];
		AdaptersList.read(FileBuf, FileSize);
		std::string Contents = FileBuf;
		
		size_t UsedAdptr = Contents.find(NAName + ":\n");
		if(UsedAdptr == std::string::npos)
		{
			printf("Could not find adapter %s, exiting\n", NAName.c_str());
			AdaptersList.close();
			return -5;
		}
		
		UsedAdptr = Contents.find("Description", UsedAdptr);
		UsedAdptr += 36;
		std::string DeviceDesc = Contents.substr(UsedAdptr, Contents.find("\n", UsedAdptr)-UsedAdptr);
		KeyLoc = Query(DeviceDesc);
		AdaptersList.close();
	}
	else
	{
		printf("Could not open adapters.list 0.o\n");
		return -6;
	}
	
	if(KeyLoc.empty())
	{
		printf("Could not find matching description in registry, exiting\n");
		return -7;
	}
	
	if(system(std::string(std::string("netsh interface set interface \"") + NAName + std::string("\" DISABLED")).c_str()) != 0)
	{
		return -8;
	}
	
	HKEY NICKey;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyLoc.c_str(), 0, KEY_WRITE, &NICKey);
	if(reset)
		RegSetValueEx(NICKey, "NetworkAddress", 0, REG_SZ, (const BYTE*)"\0", 1);
	else
		RegSetValueEx(NICKey, "NetworkAddress", 0, REG_SZ, (const BYTE*)NewMac.c_str(), NewMac.size());
	
	RegCloseKey(NICKey);
	system(std::string(std::string("netsh interface set interface \"") + NAName + std::string("\" ENABLED")).c_str());
	
	remove("adapters.list");
	return 0;
}

std::string RandMac()
{
	char WA[4] = {'2', '6', 'A', 'E'};
	srand(time(NULL));
	char AddrBuf[3];
	std::string NewMac = "";
	
	//Because of fucking wireless adapters, MAC needs this dumb format in Windows
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

std::string Query(std::string DevDesc)
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
		for(int i = 0; i < SubKeysN && RtrnStr.empty(); i++)
		{
			DescSize = 1024*sizeof(TCHAR);
			memset(DescBuf, 0, DescSize);
			RegOpenKeyEx(HKEY_LOCAL_MACHINE, std::string(std::string("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\") + KeyNames[i]).c_str(), 0, KEY_READ, &SubKey);
			long unsigned int err = RegQueryValueEx(SubKey, "DriverDesc\0", 0, 0, (unsigned char*)DescBuf, &DescSize);
			
			if(err == 0 && DevDesc == std::string(DescBuf))
			{
				RtrnStr = std::string("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\") + KeyNames[i];
			}
			RegCloseKey(SubKey);
		}
	}
	return RtrnStr;
}

bool IsElevated()
{
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if(GetTokenInformation(hToken, (TOKEN_INFORMATION_CLASS)20, &Elevation, sizeof(Elevation), &cbSize))
		{
			fRet = Elevation.TokenIsElevated;
		}
	}
	if(hToken)
	{
		CloseHandle(hToken);
	}
	return (bool)fRet;
}
/*
A Microsoft Windows process and thread batch permissions dumper with suspicious DACL alerting

Released as open source by NCC Group Plc - http://www.nccgroup.com/

Developed by Ollie Whitehouse, ollie dot whitehouse at nccgroup dot com

https://github.com/nccgroup/WindowsDACLEnumProject

Released under AGPL see LICENSE for more information
*/

#include "stdafx.h"
#include "Common.h"

#define MAGIC (0xa0000000L)
bool bExclude = true;

//
//
//
//
//
bool UsersWeCareAbout(char *lpDomain, char *lpName)
{
	
	if(strcmp(lpDomain,"NT AUTHORITY") == 0 && strcmp(lpName,"SYSTEM") ==0 ) return false;
	if(strcmp(lpDomain,"NT AUTHORITY") == 0 && strcmp(lpName,"NETWORK SERVICE") ==0 ) return false;
	if(strcmp(lpDomain,"NT AUTHORITY") == 0 && strcmp(lpName,"LOCAL SERVICE") ==0 ) return false;
	else if(strcmp(lpDomain,"BUILTIN") == 0 && strcmp(lpName,"Users") ==0) return true;
	else if(strcmp(lpDomain,"BUILTIN") == 0) return false;
	else if(strcmp(lpDomain,"NT SERVICE") == 0) return false;
	else if(strcmp(lpDomain,"NT AUTHORITY") == 0 && strcmp(lpName,"SERVICE") == 0) return false;
	else if(strcmp(lpDomain,"NT AUTHORITY") == 0 && strcmp(lpName,"INTERACTIVE") == 0) return false;
	else {
		//fprintf(stdout,"- %s we care",lpName);
		return true;
	}
}

//
//
//
//
void PrintFilePermissions(PACL DACL, bool bFile)
{

	DWORD					dwRet=0;
	DWORD					dwCount=0;
	ACCESS_ALLOWED_ACE		*ACE;
	
	// http://msdn2.microsoft.com/en-us/library/aa379142.aspx
	if(IsValidAcl(DACL) == TRUE){

		// Now for each ACE in the DACL
		for(dwCount=0;dwCount<DACL->AceCount;dwCount++){
			// http://msdn2.microsoft.com/en-us/library/aa446634.aspx
			// http://msdn2.microsoft.com/en-us/library/aa379608.aspx
			if(GetAce(DACL,dwCount,(LPVOID*)&ACE)){
				// http://msdn2.microsoft.com/en-us/library/aa374892.aspx		
				SID *sSID = (SID*)&(ACE->SidStart);
				if(sSID != NULL)
				{
					DWORD dwSize = 2048;
					char lpName[2048];
					char lpDomain[2048];
					SID_NAME_USE SNU;
					
					switch(ACE->Header.AceType){
						// Allowed ACE
						case ACCESS_ALLOWED_ACE_TYPE:
							// Lookup the account name and print it.										
							// http://msdn2.microsoft.com/en-us/library/aa379554.aspx
							if( !LookupAccountSidA( NULL, sSID, lpName, &dwSize, lpDomain, &dwSize, &SNU ) ) {
								
								DWORD dwResult = GetLastError();
								if(dwResult == ERROR_NONE_MAPPED && bExclude == true){
									break;
								} else if( dwResult == ERROR_NONE_MAPPED && bExclude == false){
									fprintf(stdout,"[i]     |\n");
									fprintf(stdout,"[i]     +-+-> Allowed 2 - NONMAPPED - SID %s\n", sidToText(sSID));
								} else if (dwResult != ERROR_NONE_MAPPED){
									fprintf(stderr,"[!] LookupAccountSid Error 	%u\n", GetLastError());
									fprintf(stdout,"[i]     |\n");
									fprintf(stdout,"[i]     +-+-> Allowed - ERROR     - SID %s\n", sidToText(sSID));
									//return;
								} else {
									continue;
								}
							} else {
								
								fprintf(stdout,"[i]     |\n");
								fprintf(stdout,"[i]     +-+-> Allowed - %s\\%s\n",lpDomain,lpName);
							}
							
							// print out the ACE mask
							fprintf(stdout,"[i]       |\n");
							fprintf(stdout,"[i]       +-> Permissions - ");
							
						
							if(bFile == false){
								if(ACE->Mask & FILE_GENERIC_EXECUTE) fprintf(stdout,",Generic Execute");
								if(ACE->Mask & FILE_GENERIC_READ   ) fprintf(stdout,",Generic Read");
								if(ACE->Mask & FILE_GENERIC_WRITE   ) fprintf(stdout,",Generic Write");
								if(ACE->Mask & GENERIC_ALL) fprintf(stdout,",Generic All");
								
								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & FILE_DELETE_CHILD)) fprintf(stdout,",Delete diretory and files - Alert");
								else if(ACE->Mask & FILE_DELETE_CHILD) fprintf(stdout,",Delete diretory and files");

								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & FILE_ADD_FILE)) fprintf(stdout,",Add File - Alert");
								else if(ACE->Mask & FILE_ADD_FILE) fprintf(stdout,",Add File");
								
								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & FILE_WRITE_EA)) fprintf(stdout,",Write Extended Attributes - Alert");
								else if(ACE->Mask & FILE_WRITE_EA) fprintf(stdout,",Write Extended Attributes");
								
								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & FILE_WRITE_ATTRIBUTES)) fprintf(stdout,",Write Attributes - Alert");
								else if(ACE->Mask & FILE_WRITE_ATTRIBUTES) fprintf(stdout,",Write Attributes");

								if(ACE->Mask & FILE_READ_EA) fprintf(stdout,",Read Extended Attributes");

								if(ACE->Mask & FILE_READ_ATTRIBUTES) fprintf(stdout,",Read Attributes");
								
								if(ACE->Mask & FILE_LIST_DIRECTORY) fprintf(stdout,",List Directory");
								if(ACE->Mask & FILE_READ_EA) fprintf(stdout,",Read Extended Attributes");
								if(ACE->Mask & FILE_ADD_SUBDIRECTORY) fprintf(stdout,",Add Subdirectory");
								
								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & FILE_TRAVERSE)) fprintf(stdout,",Traverse Directory - Alert");
								else if (ACE->Mask & FILE_TRAVERSE) fprintf(stdout,",Traverse Directory");

								if(ACE->Mask & STANDARD_RIGHTS_READ) fprintf(stdout,",Read DACL");
								if(ACE->Mask & STANDARD_RIGHTS_WRITE) fprintf(stdout,",Write DACL");
								

								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & WRITE_DAC)) fprintf(stdout,",Change Permissions - Alert");
								else if(ACE->Mask & WRITE_DAC) fprintf(stdout,",Change Permissions");
								
								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & WRITE_OWNER)) fprintf(stdout,",Change Owner - Alert");
								else if(ACE->Mask & READ_CONTROL) fprintf(stdout,",Change Owner");

								if(ACE->Mask & READ_CONTROL) fprintf(stdout,",Read Control");
								if(ACE->Mask & DELETE) fprintf(stdout,",Delete");
								if(ACE->Mask & SYNCHRONIZE) fprintf(stdout,",Synchronize");

								// http://www.grimes.nildram.co.uk/workshops/secWSNine.htm
								if(ACE->Mask & MAGIC) fprintf(stdout,",Generic Read OR Generic Write");
							} 
							else 
							{
								if(ACE->Mask & FILE_GENERIC_EXECUTE) fprintf(stdout,",Generic Execute");
								if(ACE->Mask & FILE_GENERIC_READ   ) fprintf(stdout,",Generic Read");
								if(ACE->Mask & FILE_GENERIC_WRITE   ) fprintf(stdout,",Generic Write");
								if(ACE->Mask & GENERIC_ALL) fprintf(stdout,",Generic All");

								if(ACE->Mask & FILE_GENERIC_EXECUTE) fprintf(stdout,",Execute");
								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & FILE_WRITE_ATTRIBUTES)) fprintf(stdout,",Write Attributes - Alert");
								else if(ACE->Mask & FILE_WRITE_ATTRIBUTES) fprintf(stdout,",Write Attributes");

								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & FILE_WRITE_DATA)) fprintf(stdout,",Write Data - Alert");
								else if(ACE->Mask & FILE_WRITE_DATA) fprintf(stdout,",Write Data");
								
								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & FILE_WRITE_EA)) fprintf(stdout,",Write Extended Attributes - Alert");
								else if(ACE->Mask & FILE_WRITE_EA) fprintf(stdout,",Write Extended Attributes");

								if(ACE->Mask & FILE_READ_ATTRIBUTES) fprintf(stdout,",Read Attributes");
								if(ACE->Mask & FILE_READ_DATA) fprintf(stdout,",Read Data");
								if(ACE->Mask & FILE_READ_EA) fprintf(stdout,",Read Extended Attributes");
								if(ACE->Mask & FILE_APPEND_DATA) fprintf(stdout,",Append");
								if(ACE->Mask & FILE_EXECUTE) fprintf(stdout,",Execute");

								if(ACE->Mask & STANDARD_RIGHTS_READ) fprintf(stdout,",Read DACL");
								if(ACE->Mask & STANDARD_RIGHTS_WRITE) fprintf(stdout,",Read DACL");
								
								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & WRITE_DAC)) fprintf(stdout,",Change Permissions - Alert");
								else if(ACE->Mask & WRITE_DAC) fprintf(stdout,",Change Permissions");
								
								if(UsersWeCareAbout(lpDomain,lpName) == true && (ACE->Mask & WRITE_OWNER)) fprintf(stdout,",Change Owner - Alert");
								else if(ACE->Mask & WRITE_OWNER) fprintf(stdout,",Change Owner");

								if(ACE->Mask & READ_CONTROL) fprintf(stdout,",Read Control");
								if(ACE->Mask & DELETE) fprintf(stdout,",Delete");
								if(ACE->Mask & SYNCHRONIZE) fprintf(stdout,",Synchronize");

								// http://www.grimes.nildram.co.uk/workshops/secWSNine.htm
								if(ACE->Mask & MAGIC) fprintf(stdout,",Generic Read OR Generic Write");

							}
							fprintf(stdout,"\n");
							break;
						// Denied ACE
						case ACCESS_DENIED_ACE_TYPE:
							break;
						// Uh oh
						default:
							break;
					}

					
				}
			} else {
				DWORD dwError = GetLastError();
				fprintf(stderr,"[!] Error - %d - GetAce\n", dwError);
				return;
			}
		}
	} else {
		DWORD dwError = GetLastError();
		fprintf(stderr,"[!] Error - %d - IsValidAcl\n", dwError);
		return;
	}


}

bool GetHandleBeforePrint(char* strFile){
	DWORD dwSize =0;
	DWORD dwBytesNeeded =0;
	
	GetFileSecurity (strFile,DACL_SECURITY_INFORMATION,NULL,NULL,&dwBytesNeeded);
	dwSize = dwBytesNeeded;
	PSECURITY_DESCRIPTOR* secDesc = (PSECURITY_DESCRIPTOR*)LocalAlloc(LMEM_FIXED,dwBytesNeeded);
	if(GetFileSecurity (strFile,DACL_SECURITY_INFORMATION,secDesc,dwSize,&dwBytesNeeded) == false){
		fprintf(stdout,"[i] |\n");
		fprintf(stdout,"[i] +-+-> Failed to query file system object security - %d\n",GetLastError());
		return false;
	}
	
	PACL DACL;
	BOOL bDACLPresent = false;
	BOOL bDACLDefaulted = false;


	bDACLPresent = false;
	bDACLDefaulted = false;
	if(GetSecurityDescriptorDacl(secDesc,&bDACLPresent,&DACL,&bDACLDefaulted) == false){
		fprintf(stdout,"[i] |\n");
		fprintf(stdout,"[i] +-+-> Failed to get security descriptor - %d\n",GetLastError());
		return false;
	}

	PrintFilePermissions(DACL,true);

	return true;
}
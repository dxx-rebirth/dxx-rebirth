/*
 *  extractD1Data.cpp
 *  
 *
 *  Created by Peter "halprin" Kendall on 8/26/09.
 *  Copyright 2009 @PAK Software. All rights reserved.
 *
 */

#define DEFAULT_INSTALL_LOCATION "/Volumes/Descent/Install Descent"
#define CHAOS_HOG_SIZE 0x2AA9F
#define CHAOS_HOG_OFFSET 0xAF310
#define CHAOS_MSN_REZ_SIZE 0x14C
#define CHAOS_MSN_REZ_OFFSET 0xD9E1F
#define CHAOS_MSN_SIZE 0x12B
#define CHAOS_MSN_OFFSET 0xD9F6B
#define DESCENT_HOG_SIZE 0x71C5B3
#define DESCENT_HOG_OFFSET 0xDA106
#define DESCENT_PIG_SIZE 0x3CA96D
#define DESCENT_PIG_OFFSET 0x7F6729

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

void printUsage();

int main(int argc, char **argv)
{
	string install_location=DEFAULT_INSTALL_LOCATION;
	
	if(argc>2 || (argc==2 && strcmp(argv[1], "-help")==0))
	{
		printUsage();
		return -1;
	}
	else if(argc==2)
	{
		//use a custom location for the installer
		install_location=argv[1];
	}
	
	//report what we are doing
	cout << endl;
	cout << "DETAILS" << endl;
	cout << "Installer:  " << install_location << endl;
	
	ifstream installer(install_location.c_str());
	if(!installer)
	{
		cout << "Error:  Cannot open the installer file." << endl;
		cout << "Does it exist at the location specified?"  << endl << endl;
		printUsage();
		return -2;
	}
	
	//report more on what we are doing
	cout << "CHAOS.HOG:" << endl;
	cout << "   starts 0x" << hex << uppercase << CHAOS_HOG_OFFSET << endl;
	cout << "   size 0x" << hex << uppercase << CHAOS_HOG_SIZE << endl;
	cout << "CHAOS.MSN:" << endl;
	cout << "   starts 0x" << hex << uppercase << CHAOS_MSN_OFFSET << endl;
	cout << "   size 0x" << hex << uppercase << CHAOS_MSN_SIZE << endl;
	cout << "CHAOS.MSN resource fork:" << endl;
	cout << "   starts 0x" << hex << uppercase << CHAOS_MSN_REZ_OFFSET << endl;
	cout << "   size 0x" << hex << uppercase << CHAOS_MSN_REZ_SIZE << endl;
	cout << "descent.hog:" << endl;
	cout << "   starts 0x" << hex << uppercase << DESCENT_HOG_OFFSET << endl;
	cout << "   size 0x" << hex << uppercase << DESCENT_HOG_SIZE << endl;
	cout << "descent.pig:" << endl;
	cout << "   starts 0x" << hex << uppercase << DESCENT_PIG_OFFSET << endl;
	cout << "   size 0x" << hex << uppercase << DESCENT_PIG_SIZE << endl;
	cout << endl;
	
	char* chaos_hog_buffer=(char*)malloc(CHAOS_HOG_SIZE);
	char* chaos_msn_rez_buffer=(char*)malloc(CHAOS_MSN_REZ_SIZE);
	char* chaos_msn_buffer=(char*)malloc(CHAOS_MSN_SIZE);
	char* descent_hog_buffer=(char*)malloc(DESCENT_HOG_SIZE);
	char* descent_pig_buffer=(char*)malloc(DESCENT_PIG_SIZE);
	
	ofstream chaos_hog_file("CHAOS.HOG");
	if(!chaos_hog_file)
	{
		cout << "Error:  Unable to create new CHAOS.HOG file.  Skipping!" << endl;
	}
	ofstream chaos_msn_file("CHAOS.MSN");
	if(!chaos_msn_file)
	{
		cout << "Error:  Unable to create new CHAOS.MSN file.  Skipping!" << endl;
	}
	ofstream chaos_msn_rez_file("CHAOS.MSN/..namedfork/rsrc");
	if(!chaos_msn_rez_file)
	{
		cout << "Error:  Unable to create the resource fork of the CHAOS.MSN file.  Skipping!" << endl;
	}
	ofstream descent_hog_file("descent.hog");
	if(!descent_hog_file)
	{
		cout << "Error:  Unable to create new descent.hog file.  Skipping!" << endl;
	}
	ofstream descent_pig_file("descent.pig");
	if(!descent_pig_file)
	{
		cout << "Error:  Unable to create new descent.pig file.  Skipping!" << endl;
	}
	
	//read the CHAOS.HOG file
	installer.ignore(CHAOS_HOG_OFFSET);
	installer.read(chaos_hog_buffer, CHAOS_HOG_SIZE);
	//write the CHAOS.HOG file
	chaos_hog_file.write(chaos_hog_buffer, CHAOS_HOG_SIZE);
	
	//read the CHAOS.MSN file's resource fork
	installer.ignore(CHAOS_MSN_REZ_OFFSET-(CHAOS_HOG_OFFSET+CHAOS_HOG_SIZE));
	installer.read(chaos_msn_rez_buffer, CHAOS_MSN_REZ_SIZE);
	//write the CHAOS.MSN file's resource fork
	chaos_msn_rez_file.write(chaos_msn_rez_buffer, CHAOS_MSN_REZ_SIZE);
	
	//read the CHAOS.MSN file
	installer.ignore(CHAOS_MSN_OFFSET-(CHAOS_MSN_REZ_OFFSET+CHAOS_MSN_REZ_SIZE));
	installer.read(chaos_msn_buffer, CHAOS_MSN_SIZE);
	//write the CHAOS.MSN file
	chaos_msn_file.write(chaos_msn_buffer, CHAOS_MSN_SIZE);
	
	//read the descent.hog file
	installer.ignore(DESCENT_HOG_OFFSET-(CHAOS_MSN_OFFSET+CHAOS_MSN_SIZE));
	installer.read(descent_hog_buffer, DESCENT_HOG_SIZE);
	//write the descent.hog file
	descent_hog_file.write(descent_hog_buffer, DESCENT_HOG_SIZE);
	
	//read the descent.pig file
	installer.ignore(DESCENT_PIG_OFFSET-(DESCENT_HOG_OFFSET+DESCENT_HOG_SIZE));
	installer.read(descent_pig_buffer, DESCENT_PIG_SIZE);
	//write the descent.pig file
	descent_pig_file.write(descent_pig_buffer, DESCENT_PIG_SIZE);
	
	
	//Done!
	cout << "Extraction complete!" << endl << endl;
	
	free(chaos_hog_buffer);
	free(chaos_msn_rez_buffer);
	free(chaos_msn_buffer);
	free(descent_hog_buffer);
	free(descent_pig_buffer);
	
	chaos_hog_file.close();
	chaos_msn_file.close();
	chaos_msn_rez_file.close();
	descent_hog_file.close();
	descent_pig_file.close();
	
	return 0;
}

void printUsage()
{
	cout << "Usage:  ./d1Extract <Path to Descent installer>" << endl;
	cout << "If <Path to Descent installer> is not specified, the default \"" << DEFAULT_INSTALL_LOCATION << "\" will be used." << endl << endl;
}

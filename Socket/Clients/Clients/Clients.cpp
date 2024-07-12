// Clients.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "Clients.h"
#include "afxsock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;
struct Datafile {
    char filename[20];
    char size[10];
};
//Function to turn ON/OFF cursor in console
void ShowCur(bool CursorVisibility)
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ConCurInf;

    ConCurInf.dwSize = 10;
    ConCurInf.bVisible = CursorVisibility;

    SetConsoleCursorInfo(handle, &ConCurInf);
}
//Function to call CTR + C to exit
void signal_callback_handler(int signum) {
    cout << "\nDisconnect to Server!!\n";
    // Terminate program
    exit(signum);
}
bool CheckExist(Datafile files[], char require_file[], int n) {
    for (int i = 0; i < n; i++)
        if (strcmp(require_file, files[i].filename) == 0)
            return true;
    return false;
}
//Get info from input file
void GetinfoInputFile(char require_file[][20], int& n) {
    ifstream fin("input.txt", ios::in);
    if (!fin) cout << "Cannot open file!!       ";
    else {
        int i = 0;
        string line;
        while (getline(fin, line)) {
            strcpy_s(require_file[i], line.c_str());
            i++;
        }
        n = i;
    }
    fin.close();
}
//Download file
void RecieveFile(CSocket &Client, char require_file[]) {
    string filename(require_file);
    ShowCur(0);
    int file_size, bytes_recieve;
    //Recieve file size from server
    bytes_recieve = Client.Receive((char*)&file_size, sizeof(file_size), 0);
    char buffer1[10000];
    char* buffer2;
    if (bytes_recieve != sizeof(file_size)) { cout << "Cannot download!!\n"; return; }
    //Create a new file to store data
    fstream fout("Data/" + filename, ios::out | ios::binary);
    int total_data = 0;
    cout << "......Downloading " << filename;
    while (total_data < file_size) {
        //Using char[] when it has still has more than 1028 bytes to download fastly
        if (total_data <= file_size - 10000){
            //Recieve file data from Server
            bytes_recieve = Client.Receive((char*)&buffer1, sizeof(buffer1), 0);
            if (bytes_recieve < 1) { cout << "\nCannot download!!\n"; return; }
            //Store data to new file
            fout.write((char*)&buffer1, sizeof(buffer1));
            total_data += bytes_recieve;
            //Print percent downloading
            cout << "\r" << int((total_data / float(file_size)) * 100) << "%";
        }
        //Using char* when it just has a little bit of bytes to dowload exactly
        else {
            buffer2 = new char[file_size - total_data];
            //Recieve file data from Server
            bytes_recieve = Client.Receive(buffer2, file_size - total_data, 0);
            if (bytes_recieve < 1) { cout << "\nCannot download!!\n"; return; }
            //Store data to new file
            fout.write(buffer2, file_size - total_data);
            total_data += bytes_recieve;
            //Print percent downloading
            cout << "\r" << int((total_data / float(file_size)) * 100) << "%";
        }
    }
    cout << endl;
    fout.close();
    ShowCur(1);
}
int main()
{
    ShowCur(0);
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // initialize MFC and print and error on failure
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: code your application's behavior here.
            wprintf(L"Fatal Error: MFC initialization failed\n");
            nRetCode = 1;
        }
        else
        {
            CSocket Client;
            char ipAdd[100];
            unsigned int port = 1234;
            AfxSocketInit(NULL);
            Client.Create();
            cout << "Input Server ip: "; cin.getline(ipAdd, 100);
            //Set connection to ip Address
            if (Client.Connect(CA2W(ipAdd), port)) {
                cout << "Client has been connected!!\n";
                int id;
                //Receive the number of being client
                Client.Receive((char*)&id, sizeof(id), 0);
                cout << "Client's id: " << id + 1 << endl;
                //Receive info of file can be downloaded
                Datafile files[10]; int n_list;
                Client.Receive((char*)&n_list, sizeof(n_list), 0);
                Client.Receive((char*)&files, sizeof(files), 0);
                cout << "List of file can be downloaded:\n";
                for (int i = 0; i < n_list; i++)
                    cout << files[i].filename << " " << files[i].size << endl;
                signal(SIGINT, signal_callback_handler);
                //Using idx add the number of file which had been downloaded from server
                //So, Client had't to download them again
                int idx = 0;
                //Real time downloading
                while (true) {
                    //Using flag to transfer signal when Clients is exit
                    bool flag = 0; Client.Send((char*)&flag, sizeof(flag), 0);
                    //Get info all file need to be download from input file
                    char require_file[20][20]; int list_refile;
                    GetinfoInputFile(require_file, list_refile);
                    //Download file from server
                    Client.Send((char*)&list_refile, sizeof(list_refile), 0);
                    Client.Send((char*)&require_file, sizeof(require_file), 0);
                    Client.Send((char*)&idx, sizeof(idx), 0);
                    for (; idx < list_refile; idx++) {
                        if (CheckExist(files, require_file[idx], n_list))
                            RecieveFile(Client, require_file[idx]);
                        else cout << "'" << require_file[idx] << "'" << " is not found!!\n";
                    }
                    cout << "Waiting for downloading...";
                    Sleep(5000); cout << "\r";
                }
                Client.ShutDown(2);
                Client.Close();
            }
            else cout << "Cannot connect to Server :(( \n";

        }
    }
    else
    {
        // TODO: change error code to suit your needs
        wprintf(L"Fatal Error: GetModuleHandle failed\n");
        nRetCode = 1;
    }
    getchar();
    return nRetCode;
}

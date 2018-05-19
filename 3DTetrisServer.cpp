#include "stdafx.h"

#pragma comment (lib , "ws2_32.lib")

UINT HighestScore = 0;

bool LoadHightestScoreFromFile()
{
	std::ifstream file("score.txt");

	if (file.fail()) {
		return false;
	}

	file >> HighestScore;
	return true;
}

void SaveHightestScoreToFile(char* _ip, USHORT _port)
{
	std::ofstream file("score.txt");
	file << HighestScore << " " << _ip << " " << _port;
}

UINT WINAPI WorkThread(void *_data)
{
	// Client Socket 정보를 저장해둔다.
	SOCKET sock = (SOCKET)_data;

	// Client 주소 정보를 연결된 Socket을 통해 client_addr에 저장한다.
	struct sockaddr_in client_addr;
	int addr_len = sizeof(struct sockaddr_in);
	getpeername(sock, (struct sockaddr*)&client_addr, &addr_len);
	
	// Client측의 정보(IP, Port)와 Server에서 연결에 할당한 Thread를 표기한다.
	printf_s("New Challenger Play Game : %s(%d)\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	printf_s("Allocation Thread : %d\n", GetCurrentThreadId());

	// Client는 Game이 끝나면 자신의 점수를 보내는데
	int getScore = 0;
	recv(sock, (char*)&getScore, sizeof(UINT), 0);

	// 만약 수신한 점수가 기존의 최고 점수보다 높다면
	if (getScore >= HighestScore) {
		// 최고 점수를 갱신하고 이를 표기한다.
		HighestScore = getScore;
		printf_s("New Best Player!!! : %s(%d)\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		printf_s("HightestScore : %d\n", HighestScore);
		// 최고 점수를 File에 저장한다.
		SaveHightestScoreToFile(inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	}

	// Client와의 연결을 끊고 정상적으로 Thread도 해제되는지를 표기한다.
	closesocket(sock);
	printf_s("Delete Thread : %d\n\n", GetCurrentThreadId());
	return 0;
}

int main(int _argc, char** _argv)
{
	if (LoadHightestScoreFromFile() == false) {
		printf_s("Load HightestScore from File");
		return 1;
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf_s("WSAStartup Fail");
		return 1;
	}

	SOCKET listenSocket;
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET) {
		printf_s("Create Listen Socket Fail");
		return 1;
	}

	struct sockaddr_in server_addr;
	ZeroMemory(&server_addr, sizeof(struct sockaddr_in));
	server_addr.sin_family = PF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(listenSocket, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		printf_s("Bind Listen Socket Fail");
		closesocket(listenSocket);
		return 1;
	}

	if (listen(listenSocket, 5) == SOCKET_ERROR) {
		printf_s("Wait Client Fail");
		closesocket(listenSocket);
		return 1;
	}

	printf_s("Run Server......\n");

	SOCKET clientSocket;
	struct sockaddr_in client_addr;
	ZeroMemory(&client_addr, sizeof(struct sockaddr_in));

	HANDLE hThread = nullptr;
	while (true) {
		// Client의 Accpet요청을 받고 Client와의 연결 Socket을 생성해 clientSocket에 저장한다.
		int addr_len = sizeof(struct sockaddr_in);
		clientSocket = accept(listenSocket, (struct sockaddr*) &client_addr, &addr_len);
		// Client Socket을 토대로 Client와 통신할 함수(WorkThread)를 Multi-Thread로 실행한다.
		hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, &WorkThread, reinterpret_cast<void*>(clientSocket), 0, nullptr));
		// 굳이 Main Thread에서 제어할거 아니면 ThreadHandle을 곧장 해제해준다.
		CloseHandle(hThread);
	}

	closesocket(listenSocket);
	WSACleanup();
	return 0;
}
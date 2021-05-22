#include "httpVisiting.h"

#ifdef __COMPILE_HTTP_PART

using namespace std;
#pragma warning(disable:4996)

string m_data = "http://127.0.0.1:8848/";
char m_host[256] = "";
int port = 8848;
char filename[256] = "C:/callofduty.jpeg";
FILE* fp = NULL;

int ret = -1, ContentLength = 0;
char boundary[64] = "";

char YL_head[] =	//char_count = snprintf(send_data, 4096, YL_head, m_data, m_host, boundary, ContentLength);
"POST http://127.0.0.1:8848/ HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: keep-alive\r\n"
"Content-Type: multipart/form-data; boundary=%s\r\n"
"Content-Length: %d\r\n\r\n";

char YL_request[] =	//char_count = snprintf(send_request, 4096, YL_request, boundary, filename);
"--%s\r\n"
"Content-Disposition: form-data; name=\"image\"; filename=\"%s\"\r\n"
"Content-Type: application/octet-stream;chartset=UTF-8\r\n\r\n";

char test_request[] = //char_count = snprintf(send_request, 4096, test_request, boundary, app/id, philip/1800013050);
"--%s\r\n"
"Content-Disposition: form-data; name=\"%s\"\r\n\r\n"
"%s\r\n";


int YL_image_data() { //to give the data to filepointer fp
	fp = fopen(filename, "rb");
	return 0;
}

int YL_Connect_Server() {

	gethostname( m_host , 32); //cout << "myhostname : " << m_host << endl;

	struct hostent* p_hostent = gethostbyname(m_host);													if (p_hostent == NULL) { cout << "host not exist" << endl; return -1; }

	sockaddr_in addr_server;
	addr_server.sin_family = AF_INET;
	addr_server.sin_port = htons(8848);//如果url中没有指明端口号，就设定默认的80
	memcpy(&(addr_server.sin_addr), p_hostent->h_addr_list[0], sizeof(addr_server.sin_addr));

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int res = connect(sock, (sockaddr*)&addr_server, sizeof(addr_server));										if (res == -1) { cout << "Connect failed " << endl; closesocket(sock); return -1; }

	return sock;
}

int send_http(int id, float *http_x, float *http_y) {

	char send_data[4096] = { 0 };
	char send_end[128] = { 0 };
	char send_request[2048] = { 0 };
	if (YL_image_data() < 0) { cout << "send_http : image data error" << endl; return -1; };

	WSADATA wsaData;	WSAStartup(MAKEWORD(2, 2), &wsaData);

	int sock = YL_Connect_Server();	//Connect to the Server and get a socket
	if (sock < 0) { WSACleanup(); return -1; }

	//生成boundary和ContentLength

	fseek(fp, 0, SEEK_END);
	ContentLength = ftell(fp); //send file later
	rewind(fp);

	//获取毫秒级的时间戳用于boundary的值
	SYSTEMTIME tv;
	GetLocalTime(&tv);
	long long int timestamp = (long long int)tv.wSecond * 1000 + tv.wMilliseconds;
	snprintf(boundary, 64, "---------------------------%lld", timestamp);

	// Content-Length的大小还包括了对上传文件的描述，开始boundary和结束boundary
	ContentLength += snprintf(send_request, 2048, test_request, boundary, "app", "philip");	char IDstring[16] = ""; itoa(id, IDstring, 10);
	ContentLength += snprintf(send_request, 2048, test_request, boundary, "id", IDstring);
	ContentLength += snprintf(send_request, 2048, YL_request, boundary, filename);
	ContentLength += snprintf(send_end, 128, "\r\n--%s--\r\n", boundary);

#ifdef dbgprt
	cout << "Before Sending : boundary :" << boundary << endl;
	cout << "Before Sending : length :" << ContentLength << endl;
#endif

	//发送HTTP报头
	ret = snprintf(send_data, 4096, YL_head, m_host, boundary, ContentLength);

#ifdef dbgprt
	cout << "Before Sending : " << send_data << endl;
#endif

	if (send(sock, send_data, ret, 0) != ret) //if the send char count is not expected, this action is regarded as failed
	{
		cout << "send head error!" << endl;
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	//发送测试信息
	memset(send_data, 0, sizeof(send_data));
	ret = snprintf(send_data, 2048, test_request, boundary, "app", "philip");

#ifdef dbgprt
	cout << "Sending app : " << send_data << endl;
#endif

	if (send(sock, send_data, ret, 0) != ret) //if the send char count is not expected, this action is regarded as failed
	{
		cout << "send app error!" << endl;
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	memset(send_data, 0, sizeof(send_data));
	ret = snprintf(send_data, 2048, test_request, boundary, "id", IDstring);

#ifdef dbgprt
	cout << "Sending id : " << send_data << endl;
#endif

	if (send(sock, send_data, ret, 0) != ret) { //if the send char count is not expected, this action is regarded as failed
		cout << "send id error!" << endl;
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	//发送文件

	memset(send_data, 0, sizeof(send_data));
	ret = snprintf(send_data, 2048, YL_request, boundary, filename);

#ifdef dbgprt
	cout << "Sending img : " << send_data << endl;
#endif

	if (send(sock, send_data, ret, 0) != ret) { //if the send char count is not expected, this action is regarded as failed
		cout << "send img error!" << endl;
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	while (1) {
		memset(send_data, 0, sizeof(send_data));
		ret = fread(send_data, 1, 4096, fp);					//read the file in a granularity of 4KB
		if (ret != 4096) {										//fail to read 4KB data
			if (!ferror(fp)) {									//maybe the file is about to be end
				if (send(sock, send_data, ret, 0) != ret) {		//send these data and halt if anything goes wrong
					printf("send the end data error!\n");		//anything goes wrong
					closesocket(sock); 	WSACleanup();
					fclose(fp);
					return -1;
				}
				fclose(fp);										//close the file
				break;											//and stop sending
			}
			else {												//there is something wrong in the file
				printf("read file error!\n");					//then just halt
				closesocket(sock);	WSACleanup();
				fclose(fp);
				return -1;
			}
		}

		if (send(sock, send_data, 4096, 0) != 4096) {			//read the file in a granularity of 4KB
			printf("send date error\n");						//halt if anything goes wrong
			closesocket(sock);	WSACleanup();
			fclose(fp);
			return -1;
		}

	}


	//发送结尾boundary结束传输
	memset(send_data, 0, sizeof(send_data));
	ret = snprintf(send_data, 4096, "\r\n--%s--\r\n", boundary);
	if (send(sock, send_data, ret, 0) != ret)
	{
		cout << "send ending error" << endl;
		closesocket(sock); 	WSACleanup();
		return -1;
	}

	string  m_readBuffer;																						if (m_readBuffer.empty())	m_readBuffer.resize(512);
	int readCount = recv(sock, &m_readBuffer[0], m_readBuffer.size(), 0);

	sscanf( m_readBuffer.c_str(), "%f %f", http_x, http_y);

#ifdef dbgprt
	cout << http_x << endl;
	cout << http_y << endl;
	cout << "Request: " << m_data << " and response:" << m_readBuffer << endl;
#endif

	closesocket(sock); 	WSACleanup();
	return 0;
}
#endif



//Philip：在这一段考虑用POST进行传输时的数据结构，对应眼动预测模型

	//发一系列关于用户和物体的信息，一共从现在往前推50帧，共11项内容需要记录，因此这里的数组长度为50*11=5050 （默认值）
		//或者我们通过数据库的数据结构来操作它？使得我们只需要发送最后一帧的那11项内容就可以了

	//发一张图片，用于生成saliency map；大小不知道，因为我们的眼动预测算法接受的输入直接就是saliency map，它应该是进行了离线计算
		//在线计算的话那还得内置一个saliency network在那里
	//如果只形式上要成功的话就输出两个24*24的图片给模型即可

//Philip：在这一段考虑用POST进行传输时的数据结构，对应眼动预测模型
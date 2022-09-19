#include "../utils/security_utils.h"

class Client {
    string username;
    //EVP_PKEY *my_priv_key;  // togliere?
    Session *active_session;

    //unsigned char *send_buffer;
    //unsigned char *recv_buffer;
    array<unsigned char, MAX_BUF_SIZE> send_buffer;
    array<unsigned char, MAX_BUF_SIZE> recv_buffer;

    /********* socket *********/
    int sd; // socket descriptor
    sockaddr_in sv_addr;

    void createSocket(string srv_ip);
    /****************************/

    /***** utility methods *****/

    // new sed/receive
    int sendMsg(int payload_size);
    long receiveMsg();


    void sendErrorMsg(string errorMsg);
    
    bool verifyCert(unsigned char* buffer_cert, long cert_size, EVP_PKEY*& srv_pubK); // verify certificate -> build store -> load cert&crl -> extract pubK
    void buildStore(X509*& ca_cert, X509_CRL*& crl, X509_STORE*& new_store);


    // methods invoked during the authentication phase -> never called from outside class -> can be private
    int sendUsername();
    bool receiveCertSign(unsigned char* srv_nonce);    // receive (nonce, ecdh_key, cert, dig_sign), deserialize and verify server cert and digital signature
    void sendSign(unsigned char* srv_nonce, EVP_PKEY *priv_k);

    // methods invoked by handlerCommand method -> only from inside -> can be private
    void requestFileList();
    void receiveFileList();
    void logout();  // dealloca tutto ed esce
    // TODO: implementare/modificare come serve 
        // (li scrivo solo per evitare conflitti su git, ci sono anche le definizioni nel file .cpp)
    void uploadFile();
    void downloadFile();
    void renameFile();
    void deleteFile();

    public:
        Client(string username, string srv_ip);
        ~Client();

        bool authentication();  // call session.generatenonce, sendMsg, receive
        void showCommands();
        bool handlerCommand(string& command);
        
};
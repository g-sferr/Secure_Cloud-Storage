#include "client.h"

Client::Client(string username, string srv_ip) {
    this->username = username;
    active_session = new Session();
    /*
    send_buffer = (unsigned char*)malloc(MAX_BUF_SIZE);
    if(!send_buffer)
        handleErrors("Malloc error");
    recv_buffer = (unsigned char*)malloc(MAX_BUF_SIZE);
    if(!recv_buffer)
        handleErrors("Malloc error");
        */
    createSocket(srv_ip);
}

Client::~Client() {
    username.clear();
    if(!send_buffer.empty()) {
        send_buffer.fill('0');
        //free(send_buffer);
        //send_buffer = nullptr;
    }
    if(!recv_buffer.empty()) {
        recv_buffer.fill('0');
        //free(recv_buffer);
        //recv_buffer = nullptr;
    }
}

void Client::createSocket(string srv_ip) {
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)  // socket TCP
        handleErrors("Socket creation error");
    memset(&sv_addr, 0, sizeof(sv_addr));
    sv_addr.sin_family = AF_INET;
    sv_addr.sin_port = htons(SRV_PORT);
    if(inet_pton(AF_INET, srv_ip.c_str(), &sv_addr.sin_addr) != 1)
        handleErrors("Server IP not valid");
    if(connect(sd, (struct sockaddr*)&sv_addr, sizeof(sv_addr)) != 0)
        handleErrors("Connection to server failed");
}

/********************************************************************/
// send/receive alternative version

/**
 * send a message to the server
 * @payload_size: body lenght of the message to send
 * @return: 1 on success, 0 or -1 on error
 */
int Client::sendMsg(int payload_size) {
    cout << "sendMsg new" << endl;
    if(payload_size > MAX_BUF_SIZE - NUMERIC_FIELD_SIZE) {
        cerr << "Message to send too big" << endl;
        send_buffer.fill('0');
        //memset(send_buffer, 0, MAX_BUF_SIZE);
        return -1;
    }
    payload_size += NUMERIC_FIELD_SIZE;
    if(send(sd, send_buffer.data(), payload_size, 0) < payload_size) {
        perror("Socker error: send message failed");
        send_buffer.fill('0');
        //memset(send_buffer, 0, MAX_BUF_SIZE);
        return -1;
    }
    send_buffer.fill('0');
    //memset(send_buffer, 0, MAX_BUF_SIZE);

    return 1;    
 }

/**
 * receive message from server
 * @return: return the payload length of the received message, or 0 or -1 on error
*/
 long Client::receiveMsg() {
    cout << "receiveMsg new" << endl;

    ssize_t msg_size = 0;

    recv_buffer.fill('0');
    //memset(recv_buffer, 0, MAX_BUF_SIZE);
    msg_size = recv(sd, recv_buffer.data(), MAX_BUF_SIZE-1, 0);
    cout << "received msg size: " << msg_size << endl;

    if (msg_size == 0) {
        cerr << "The connection has been closed" << endl;
        return 0;
    }

    if (msg_size < 0 || msg_size < (unsigned int)NUMERIC_FIELD_SIZE) {
        perror("Socket error: receive message failed");
        recv_buffer.fill('0');
        //memset(recv_buffer, 0, MAX_BUF_SIZE);
        return -1;
    }
    uint32_t payload_n = *(unsigned int*)(recv_buffer.data());
    uint32_t payload_size = ntohl(payload_n);
    cout << payload_size << " received payload length" << endl;
    //check if received all data
    if (payload_size != msg_size - NUMERIC_FIELD_SIZE) {
        cerr << "Error: Data received too short (malformed message?)" << endl;
        recv_buffer.fill('0');
        //memset(recv_buffer, 0, MAX_BUF_SIZE);
        return -1;
    }

    return payload_size;

 }

/********************************************************************/


bool Client::authentication() {
    cout << "Client->autentication\n";
    // M1
    if(sendUsername() != 1) {
        cerr << "Authentication failed" << endl;
    }
    
    /*
    int start_index = 0;
    uint32_t payload_size = OPCODE_SIZE + NONCE_SIZE + username.size();

    memset(send_buffer, 0, MAX_BUF_SIZE);
    active_session->generateNonce();
    // prepare buffer: | payload_size | opcode | nonce_client | username |
    uint32_t payload_n = htonl(payload_size);
    memcpy(send_buffer, (unsigned char*)&payload_n, NUMERIC_FIELD_SIZE);
    start_index += NUMERIC_FIELD_SIZE;
    uint16_t opcode = htons((uint16_t)LOGIN);
    memcpy(send_buffer + start_index, (unsigned char*)&opcode, OPCODE_SIZE);
    start_index += OPCODE_SIZE;
    memcpy(send_buffer + start_index, active_session->nonce.data(), NONCE_SIZE);
    start_index += NONCE_SIZE;
    memcpy(send_buffer + start_index, username.c_str(), username.size());
    start_index += username.size();
    //sendMsg
    cout << "authentication->sendMsg (nonce, usr)" << endl;
    sendMsg(payload_size);     // dimensione del messaggio da inviare -> solo payload, l'header viene aggiunto in sendMsg
    */
    /*BIO_dump_fp(stdout, (const char*)send_buffer, start_index);    // stampa in esadecimale
    cout << payload_size << " buffer len" << strlen((char*)send_buffer) << endl;
    for(int i=0; i<start_index; i++) {
        cout << send_buffer[i];
    }
    cout << endl;*/

    // M2: receive server cert e ECDH_server_key
    cout << "authentication->receiveMsg" << endl;
    array<unsigned char, NONCE_SIZE> server_nonce;
    /*unsigned char* srv_nonce = (unsigned char*)malloc(NONCE_SIZE);
    if(!srv_nonce) {
        cerr << "Malloc failed " << endl;
        
        return false;
    }
    */
    // receive M2
    if(!receiveCertSign(server_nonce.data())) {
        cerr << "receiveVerifyCert failed" << endl;
        return false;
    }
    /*
    //start_index = 0;
    int received_size = receiveMsg(payload_size);    // return total size received data

    active_session->deserializeKey(ECDH_srv_key, ECDH_key_size, active_session->ECDH_peerKey);
    */
    // DONE legge/deserializza msg -> verifica nonce -> verifica cert server -> verifica firma server -> 
    //genera ECDH_key -> prepara buffer&invia -> riceve login ack
    
    EVP_PKEY *my_priv_key;
    active_session->retrievePrivKey("./client/users/" + username + "/" + username + "_key.pem", my_priv_key);
    active_session->generateECDHKey();
    sendSign(server_nonce.data(), my_priv_key);
    cout << "sendsign serv nonce" << endl;
    server_nonce.fill('0');
    /*
    unsigned char* ECDH_my_pub_key = NULL;
    unsigned int ECDH_my_key_size = active_session->serializeKey(active_session->ECDH_myKey, ECDH_my_pub_key);
    */

    active_session->deriveSecret();     // derive secrete & compute session key
    cout << "active_session -> derive secret " << endl;
    cout << "authentication -> receive users list" << endl;
    // TODO
    //receive login ack or file list?
    //receiveFileList();
    return true;
}

/********************************************************************/

// Message M1
int Client::sendUsername() {
    cout << "sendUsername\n";
    int start_index = 0;
    uint32_t payload_size = OPCODE_SIZE + NONCE_SIZE + username.size();
    uint32_t payload_n = htonl(payload_size);
    send_buffer.fill('0');
    //memset(send_buffer, 0, MAX_BUF_SIZE);
    active_session->generateNonce();
    // prepare buffer: | payload_size | nonce_client | username |
    memcpy(send_buffer.data(), (unsigned char*)&payload_n, NUMERIC_FIELD_SIZE);
    start_index += NUMERIC_FIELD_SIZE;
    memcpy(send_buffer.data() + start_index, active_session->nonce.data(), NONCE_SIZE);
    start_index += NONCE_SIZE;
    memcpy(send_buffer.data() + start_index, username.c_str(), username.size());
    start_index += username.size();
    //sendMsg
    cout << "authentication->sendMsg (nonce, usr)" << endl;
    if(sendMsg(payload_size) != 1) {
        active_session->nonce.fill('0');
        return -1;
    }
    return 1;
}

// M2
bool Client::receiveCertSign(unsigned char *srv_nonce) {
    cout << "receiveCertSign\n";
    if(!srv_nonce)
        return false;
    int start_index = 0;
    int payload_size =  receiveMsg();

    start_index = NUMERIC_FIELD_SIZE;   // reading starts after payload_size field

    // check opcode
    uint16_t opcode_n = *(unsigned short*)(recv_buffer.data() + start_index);
    uint16_t opcode = ntohs(opcode_n);
    start_index += OPCODE_SIZE;
    cout << "start index " << start_index << endl;
    if(opcode != LOGIN) {
        if(opcode == ERROR) {
            string errorMsg((const char*)recv_buffer.data() + start_index, payload_size - OPCODE_SIZE);
            cerr << errorMsg << endl;
        } else {
            cerr << "Received not expected message" << endl;
        }
        
        recv_buffer.fill('0');
        /*
        #pragma optimize("", off);
            memset(recv_buffer, 0, MAX_BUF_SIZE);
        #pragma optimize("", on);
        
        free(recv_buffer);
        free(srv_nonce);
        */

        return false;
    }
    //cout << opcode << " received opcode client" << endl;

    // retrieve & check client nonce
    array<unsigned char, NONCE_SIZE> received_nonce;
    memcpy(received_nonce.data(), recv_buffer.data() + start_index, NONCE_SIZE);
    /*
    unsigned char *nonce = (unsigned char*)malloc(NONCE_SIZE);
    if(!nonce) {
        cerr << "Malloc nonce failed" << endl;
        //fare funzione che svuota i vari buffer usati, per evitare troppa ridondanza
        return false;
    }
    */

    memcpy(received_nonce.data(), recv_buffer.data() + start_index, NONCE_SIZE);   // client nonce
    start_index += NONCE_SIZE;
    if(!active_session->checkNonce(received_nonce.data())) {
        cerr << "Received nonce not valid\n";
        received_nonce.fill('0');
        // deallocare anche gli altri buffer
        recv_buffer.fill('0');
        /*
        #pragma optimize("", off);
            memset(recv_buffer, 0, MAX_BUF_SIZE);
        #pragma optimize("", on);
        
        //free(nonce);
        free(recv_buffer);
        //free(srv_nonce);
        */
        return false;
    }
    cout << "Received nonce verified!" << endl;
    // memset(nonce, 0, NONCE_SIZE);
    //free(nonce);
    // retrieve server nonce
    memcpy(srv_nonce, recv_buffer.data() + start_index, NONCE_SIZE);   // server nonce
    start_index += NONCE_SIZE;

    // retrieve server cert
    int cert_size_n = *(unsigned int*)(recv_buffer.data() + start_index);
    int cert_size = ntohl(cert_size_n);
    start_index += NUMERIC_FIELD_SIZE;
    
    array<unsigned char, MAX_BUF_SIZE> temp_buffer;

    /*unsigned char *server_cert = (unsigned char*)malloc(cert_size);
    if(!server_cert) {
        cerr << "Malloc server_cert failed" << endl;

        #pragma optimize("", off);
            memset(recv_buffer, 0, MAX_BUF_SIZE);
        #pragma optimize("", on);
        
        //free(nonce);
        free(recv_buffer);
        //free(srv_nonce);
        return false;
    }*/
    memcpy(temp_buffer.data(), recv_buffer.data() + start_index, cert_size);
    start_index += cert_size;

    // deserialize, verify certificate & extract server pubKey
    EVP_PKEY* srv_pubK;
    if(!verifyCert(temp_buffer.data(), cert_size, srv_pubK)) {
        cerr << "Server certificate not verified\n";

        recv_buffer.fill('0');
        /*
        #pragma optimize("", off);
            memset(recv_buffer, 0, MAX_BUF_SIZE);
        #pragma optimize("", on);

        EVP_PKEY_free(srv_pubK);
        //free(server_cert);
        //free(nonce);
        free(recv_buffer);
        //free(srv_nonce);
        */
        return false;
    }
    cout << "Server certificate verified!" << endl;
    temp_buffer.fill('0');   //once verified, the certificate can be deleted -> array "reset"

    // retrieve ECDH server pub key
    uint32_t ECDH_key_size_n = *(unsigned int*)(recv_buffer.data() + start_index);
    uint32_t ECDH_key_size = ntohl(ECDH_key_size_n);
    start_index += NUMERIC_FIELD_SIZE;
    array<unsigned char, MAX_BUF_SIZE> ECDH_server_key;
    /*
    unsigned char *ECDH_srv_key = (unsigned char*)malloc(ECDH_key_size);
    if(!ECDH_key_size)
        handleErrors("Malloc error");
        */
    memcpy(ECDH_server_key.data(), recv_buffer.data() + start_index, ECDH_key_size);
    start_index += ECDH_key_size;

    // retrieve digital signature
    int dig_sign_len = payload_size + NUMERIC_FIELD_SIZE - start_index; //*(unsigned int*)(recv_buffer + start_index);
    if(dig_sign_len <= 0) {
        cerr << "Dig_sign length error " << endl;
        ECDH_server_key.fill('0');
        return false;
    }

    vector<unsigned char> server_dig_sign;

    /*
    unsigned char *dig_sign = (unsigned char*)malloc(dig_sign_len);
    if(!dig_sign)
        handleErrors("Malloc error");
        */
        server_dig_sign.insert(server_dig_sign.begin(), recv_buffer.begin() + start_index, 
            recv_buffer.begin() + start_index + dig_sign_len);
    //memcpy(server_dig_sign.data(), recv_buffer. + start_index, dig_sign_len);
    start_index += dig_sign_len;
    if(start_index - NUMERIC_FIELD_SIZE != payload_size) {
        cerr << "Received data size error" << endl;
        ECDH_server_key.fill('0');
        server_dig_sign.clear();
    }
    
    // verify digital signature
    uint32_t signed_msg_len = NONCE_SIZE + ECDH_key_size;

    /*
    unsigned char* signed_msg = (unsigned char*)malloc(signed_msg_len);
    if(!signed_msg)
        handleErrors("Malloc error");
        */
    // TODO: aggiungere std::vector<unsigned char> signed_msg; al posto di temp_buffer, 
    //  dopo aver sostituito i buffer send e receive con std::array per poter usare gli iterator
    // nonce client
    memcpy(temp_buffer.data(), active_session->nonce.data(), NONCE_SIZE);
    start_index = NONCE_SIZE;
    // server ECDH public key
    memcpy(temp_buffer.data() + start_index, ECDH_server_key.data(), ECDH_key_size);
    if(!active_session->verifyDigSign(server_dig_sign.data(), dig_sign_len, srv_pubK, temp_buffer.data(), signed_msg_len)) {
        cerr << "Digital Signature not verified" << endl;
        return false;
    }
    cout << " Digital Signature Verified!" << endl;
    //free(signed_msg);
    BIO_dump_fp(stdout, (const char*) ECDH_server_key.data(), ECDH_key_size);
    active_session->deserializePubKey(ECDH_server_key.data(), ECDH_key_size, active_session->ECDH_peerKey);
    return true;
}

void Client::sendSign(unsigned char* srv_nonce, EVP_PKEY *priv_k) {
    unsigned char* ECDH_my_pub_key = NULL;
    unsigned int ECDH_my_key_size = active_session->serializePubKey(active_session->ECDH_myKey, ECDH_my_pub_key);
    
    unsigned char* msg_to_sign = (unsigned char*)malloc(NONCE_SIZE + ECDH_my_key_size);
    if(!msg_to_sign)
        handleErrors("Malloc error");
    memcpy(msg_to_sign, srv_nonce, NONCE_SIZE);
    memcpy(msg_to_sign + NONCE_SIZE, ECDH_my_pub_key, ECDH_my_key_size);
    unsigned char* signed_msg = NULL;
    int signed_msg_len = active_session->signMsg(msg_to_sign, NONCE_SIZE + ECDH_my_key_size, priv_k, signed_msg);

    // prepare send buffer
    send_buffer.fill('0');
    //memset(send_buffer, 0, MAX_BUF_SIZE);
    uint32_t payload_size = OPCODE_SIZE + NONCE_SIZE + NUMERIC_FIELD_SIZE + ECDH_my_key_size + signed_msg_len;
    uint32_t payload_n = htonl(payload_size);
    memcpy(send_buffer.data(), (unsigned char*)&payload_n, NUMERIC_FIELD_SIZE);
    int start_index = NUMERIC_FIELD_SIZE;
    uint16_t opcode = htons((uint16_t)LOGIN);
    memcpy(send_buffer.data() + start_index, (unsigned char*)&opcode, OPCODE_SIZE);
    start_index += OPCODE_SIZE;
    memcpy(send_buffer.data() + start_index, srv_nonce, NONCE_SIZE);
    start_index += NONCE_SIZE;
    uint32_t ECDH_my_key_size_n = htonl(ECDH_my_key_size);
    memcpy(send_buffer.data() + start_index, (unsigned char*)&ECDH_my_key_size_n, NUMERIC_FIELD_SIZE);
    start_index += NUMERIC_FIELD_SIZE;
    memcpy(send_buffer.data() + start_index, ECDH_my_pub_key, ECDH_my_key_size);
    start_index += ECDH_my_key_size;
    memcpy(send_buffer.data() + start_index, signed_msg, signed_msg_len);

    // send msg to server
    cout <<"authentication sendMsg (ecdh pub key)" << endl;
    sendMsg(payload_size);
    cout << "sendSign end" << endl;
}


/********************************************************************/

void Client::buildStore(X509*& ca_cert, X509_CRL*& crl, X509_STORE*& store) {
    // load CA certificate
    string ca_cert_filename = "./client/FoundationOfCybersecurity_cert.pem";    // controllare percorso directory
    FILE* ca_cert_file = fopen(ca_cert_filename.c_str(), "r");
    if(!ca_cert_file) {
        handleErrors("CA_cert file doesn't exist");
    }
    
    ca_cert = PEM_read_X509(ca_cert_file, NULL, NULL, NULL);
    fclose(ca_cert_file);
    if(!ca_cert)
        handleErrors("PEM_read_X509 returned NULL");

    // load the CRL
    string crl_filename = "./client/FoundationOfCybersecurity_crl.pem";
    FILE* crl_file = fopen(crl_filename.c_str(), "r");
    if(!crl_file) 
        handleErrors("CRL file doesn't exist");
    
    crl = PEM_read_X509_CRL(crl_file, NULL, NULL, NULL);
    fclose(crl_file);
    if(!crl)
        handleErrors("PEM_read_X509_CRL returned NULL");

    // build a store with CA_cert and the CRL
    store = X509_STORE_new();
    if(!store) {
        string err = "X509_STORE_new returned NULL\n";
        err.append(ERR_error_string(ERR_get_error(), NULL));
        handleErrors(err.c_str());
    }
    if(X509_STORE_add_cert(store, ca_cert) != 1) {
        string err = "X509_STORE_add_cert error\n";
        err.append(ERR_error_string(ERR_get_error(), NULL));
        handleErrors(err.c_str());
    }
    if(X509_STORE_add_crl(store, crl) != 1) {
        string err = "X509_STORE_add_crl error\n";
        err.append(ERR_error_string(ERR_get_error(), NULL));
        handleErrors(err.c_str());
    }
    if(X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK) != 1) {
        string err = "X509_STORE_set_flags error\n";
        err.append(ERR_error_string(ERR_get_error(), NULL));
        handleErrors(err.c_str());
    }
}

bool Client::verifyCert(unsigned char* cert_buf, long cert_size, EVP_PKEY*& srv_pubK) {
    X509* certToCheck = d2i_X509(NULL, (const unsigned char**)&cert_buf, cert_size);
    if(!certToCheck)
        handleErrors("d2i_X509 failed");

    bool verified = false;

    X509* CA_cert;
    X509_CRL * CA_crl;
    X509_STORE* store;

    if(!certToCheck)
        handleErrors("Nothing to check");

    buildStore(CA_cert, CA_crl, store); 

    // verify peer's certificate
    X509_STORE_CTX* certvfy_ctx = X509_STORE_CTX_new();
    if(!certvfy_ctx) {
        string err = "X509_STORE_CTX_new returned NULL\n";
        err.append(ERR_error_string(ERR_get_error(), NULL));
        handleErrors(err.c_str());
    }
    if(X509_STORE_CTX_init(certvfy_ctx, store, certToCheck, NULL) != 1) {
        string err = "X50_STORE_CTX_init error\n";
        err.append(ERR_error_string(ERR_get_error(), NULL));
        handleErrors(err.c_str());
    }
    verified = X509_verify_cert(certvfy_ctx) == 1;

    if(verified) {        
        srv_pubK = X509_get_pubkey(certToCheck);    // extract server public key from certificate
        // print the successful verification to screen
            //oneline -> distinguished name
        char *tmp = X509_NAME_oneline(X509_get_subject_name(certToCheck), NULL, 0);
        char *tmp2 = X509_NAME_oneline(X509_get_issuer_name(certToCheck), NULL, 0);
        cout << "Certificate of '" << tmp << "' (released by '" << tmp2 << "') verified successfully\n";
        free(tmp);
        free(tmp2);
    }

    X509_free(certToCheck);
    X509_STORE_free(store); // deallocates also CA_cert and CRL
    X509_STORE_CTX_free(certvfy_ctx);

    return verified;
}

void Client::showCommands() {
    cout << "\n-----------------------------------------------\n";
    cout << "Commands menu" << endl;
    cout << "!help -> show this commands list" << endl;
    cout << "!list -> show available file list" << endl;
    cout << "!upload -> upload an existing file in your cloud storage" << endl;
    cout << "!download -> download a file from your cloud storage" << endl;
    cout << "!rename -> rename a file in your cloud storage" << endl;
    cout << "!delete -> delete a file from your cloud storage" << endl;
    cout << "!exit -> logout from server and exit program" << endl;

}

// TODO
bool Client::handlerCommand(string& command) {
    cout << "client->hanlerCommand\n";
    //if else con la gestione dei diversi comandi, es. se rtt => readInput per leggere l'username con cui si vuole chattare (lato server va controllato che il nome sia corretto)
    if(command.compare("!help") == 0)
        showCommands();
    else if(command.compare("!list") == 0) {
        // opcode 8
        requestFileList();
        /*
        string msg = "Available users?";
        active_session->userList((unsigned char*)msg.c_str(), msg.length());*/
        // se unsigned char msg[] => active_session->userList(msg, sizeof(msg));
    } else if(command.compare("!upload") == 0) {
        // opcode 1
        // TODO
        uploadFile();    // username saved in class member
    } else if(command.compare("!download") == 0) {
        // opcode 2
        // TODO
        downloadFile();
    } else if(command.compare("!rename") == 0) {
        // opcode 3
        // TODO
        renameFile();    // username saved in class member
    } else if(command.compare("!delete") == 0) {
        // opcode 4
        // TODO
        deleteFile();
    } else if(command.compare("!exit") == 0) {
        // logout from server - opcode 10
        // TODO
        logout();        
    } else {
        cout << "Invalid command" << endl;
        return false;
    }
    return true;
}

void Client::requestFileList() {
    cout << "client -> fileList\n";
    // opcode 2
    string msg = "Available files?";
    send_buffer.fill('0');
    //memset(send_buffer, 0, MAX_BUF_SIZE);
    int payload_size = active_session->fileList((unsigned char*)msg.c_str(), msg.length(), send_buffer.data());    // prepare msg to send
    sendMsg(payload_size);

    receiveFileList();
}

// TODO
void Client::receiveFileList() {
    unsigned char *aad, *user_list;
    int aad_len;
    int payload_size = receiveMsg();
    //int received_size = receiveMsg(payload_size);
    int list_len = active_session->decryptMsg(recv_buffer.data() + NUMERIC_FIELD_SIZE, payload_size, aad, aad_len, user_list);
    uint16_t opcode_n = *(unsigned short*)(aad + NUMERIC_FIELD_SIZE);
    uint16_t opcode = ntohs(opcode_n);
    if(opcode == 0) {
        string errorMsg((const char*)user_list, strlen((char*)user_list));
        cerr << errorMsg << endl;
        handleErrors("Error opcode");
    } else if(opcode != FILE_LIST)
        handleErrors("Opcode error, message tampered");

    if(list_len != strlen((char*)user_list)) {
        cerr << "received list damaged" << endl;
        return;
    }
    string list((const char*)user_list);    // users list already formatted by the server
    cout << "Available files:\n" << list << endl;
}

// TODO
void Client::logout() {
    // deallocare tutte le risorse utilizzate e chiudere il socket di connessione col server

}

void Client::sendErrorMsg(string errorMsg) {
        //cerr << errorMsg << endl;

        // inviare mess errore al client
        int payload_size = OPCODE_SIZE + errorMsg.size();
        uint16_t op = ERROR;

        int written = 0;
        memcpy(send_buffer.data(), &payload_size, NUMERIC_FIELD_SIZE);
        written += NUMERIC_FIELD_SIZE;
        memcpy(send_buffer.data() + written, &op, OPCODE_SIZE);
        written += OPCODE_SIZE;
        memcpy(send_buffer.data() + written, errorMsg.c_str(), errorMsg.size());
        
        sendMsg(payload_size);

}

void Client::uploadFile(){}

void Client::downloadFile(){}

void Client::renameFile(){}

void Client::deleteFile(){}
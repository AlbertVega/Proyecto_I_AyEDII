#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include "bmp.h"

using namespace std;

int bmp::getWidth() const
{
    return width;
}

int bmp::getHeight() const
{
    return height;
}

unsigned char *bmp::readBMP(string filename)
{
    FILE* f = fopen(filename.c_str(), "rb");
    unsigned char info[54];
    fread(&info, sizeof(unsigned char), 54, f);

    if (info[0] != 0x42 && info[1] != 0x4d) {
        cout<< "Formato de archivo invalido";
        return NULL;
    }

    int width, height;
    width = *(int*)&info[18];
    height = *(int*)&info[22];

    fileinfo.width = width;
    this->width = width;
    fileinfo.height = height;
    this->height = height;

    int size = width * height * 3;

    unsigned char* data = new unsigned char[size];

    fread(data, sizeof(unsigned char), size, f);
    fclose(f);

    for (int i = 0; i < size; i+=3) {
        unsigned char tmp = data[i];
        data[i] = data[i+2];
        data[i+2] = tmp;
    }
    return data;
}


int main() {
    boost::asio::io_service io_service; // Servicio de input/output
    boost::asio::ip::tcp::socket socket(io_service); // Declaracion de socket para conexiones
    boost::system::error_code error; // Variable para codigo de error especifico de Boost
    boost::asio::streambuf receive_buffer; // Buffer para recibir mensajes
    const string outMessage = "Hola, soy el cliente \n"; // Mensaje a enviar

    socket.connect(boost::asio::ip::tcp::endpoint( boost::asio::ip::address::from_string("127.0.0.1"), 1234));
                                                                       // Conecta socket a IP local con puerto del servidor
    cout << "Conectado al servidor" << endl;

    //escritura del mensaje a enviar al server*/

    boost::asio::write(socket, boost::asio::buffer(data), error); // Escribe mensaje al servidor
    
    if(!error) {
        cout << "Mensaje enviado" << endl;
    }
    else {
        cout << "error" << error.message() << endl;
    }

    //lectura del mensaje enviado por el server

    boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error); // Guarda mensaje recibido en el buffer

    if (error && error != boost::asio::error::eof) {
        cout << "error" << error.message() << endl;
    } else {
        const char* inMessage = boost::asio::buffer_cast<const char*>(receive_buffer.data()); // Interpreta mensaje recibido
        cout << "Servidor dice: " + (string) inMessage << endl; // Se hace cast a string al inMessage ya que es un const char pointer,
                                                                  // para poder concatenarlo basicamente
    }
    return 0;
}
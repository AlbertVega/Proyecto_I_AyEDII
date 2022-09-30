#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/stitching.hpp"



#include <opencv2/core.hpp>
#include <boost/serialization/split_free.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>



#include <string>
#include <vector>
using namespace std;
using namespace cv;

/*
 * serialization
 */
BOOST_SERIALIZATION_SPLIT_FREE( cv::Mat )

namespace boost {
    namespace serialization {

        template <class Archive>
        void save( Archive & ar, const cv::Mat & m, const unsigned int version )
        {
            size_t elemSize = m.elemSize();
            size_t elemType = m.type();

            ar & m.cols;
            ar & m.rows;
            ar & elemSize;
            ar & elemType;

            const size_t dataSize = m.cols * m.rows * m.elemSize();
            for ( size_t i = 0; i < dataSize; ++i )
                ar & m.data[ i ];
        }


        template <class Archive>
        void load( Archive & ar, cv::Mat& m, const unsigned int version )
        {
            int cols, rows;
            size_t elemSize, elemType;

            ar & cols;
            ar & rows;
            ar & elemSize;
            ar & elemType;

            m.create( rows, cols, static_cast< int >( elemType ) );
            const size_t dataSize = m.cols * m.rows * elemSize;
            for (size_t i = 0; i < dataSize; ++i)
                ar & m.data[ i ];
        }

    } // namespace serialization
} // namespace boost

std::string save( const cv::Mat & mat )
{
    std::ostringstream oss;
    boost::archive::text_oarchive toa( oss );
    toa << mat;

    return oss.str();
}

void load( cv::Mat & mat, const char * data_str )
{
    std::stringstream ss;
    ss << data_str;

    boost::archive::text_iarchive tia( ss );
    tia >> mat;
}

/**
 *
 * Funcion que lee mensaje enviado por cliente
 *
 * @param socket
 * @return
 */
string ReadMessage(boost::asio::ip::tcp::socket & socket) {
    boost::asio::streambuf buf; // Buffer de entrada de mensajes
    boost::asio::read_until( socket, buf, "\n" ); //  Indica que lea mensaje del socket desde el buffer hasta el delimitador \n
    string data = boost::asio::buffer_cast<const char*>(buf.data()); // Hace cast del buffer de entrada a un char pointer (caracteres legibles)
    return data; // Retorna el mensaje recibido
}
/**
 *
 * Funcion que envia mensaje al cliente
 *
 * @param socket
 * @param message
 */
void SendMessage(boost::asio::ip::tcp::socket & socket, string message) {
    string msg = message + "\n"; // Declara variable string con un delimitador linea siguiente
    boost::asio::write( socket, boost::asio::buffer(msg)); // Envia mensaje a cliente mediante buffer
}

int main() {
    boost::asio::io_service io_service; // Servicio de input/output
    boost::asio::ip::tcp::acceptor acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),
                                                                                        1234));// Acepta de manera asincrona conexiones en puerto 1234
    boost::asio::ip::tcp::socket socket_(io_service); // Declaracion de socket para conexiones
    boost::system::error_code error;

    cout << "Servidor iniciado" << endl;
    acceptor_.accept(socket_); // Acepta al socket del cliente que pida conectarse
    cout << "Cliente conectado" << endl;

    string sizeMessage = ReadMessage(socket_);
    sizeMessage.pop_back(); // Popping last character "\n"
    SendMessage(socket_, "Tamaño recibido");
    int size = stoi(sizeMessage);

    vector<cv::Mat> blocks;

    for (int i = 0; i < size; i++) {
        string message = ReadMessage(socket_); // Lee y declara mensaje del cliente
        SendMessage(socket_, "Pedazo " + to_string(i) + " recibido");
        message.pop_back();
        Mat result;
        load(result, message.c_str());
        //imshow("prueba", result);
        //waitKey(0);
        blocks.push_back(result);
    }

    /*
     * REBUILD
     */

    cv::utils::fs::createDirectory("Result");
    for (int j = 0; j < blocks.size(); j++)
    {
        std::string blockId = std::to_string(j);
        std::string blockImgName = "Result/block#" + blockId + ".jpeg";
        imwrite(blockImgName, blocks[j]);
    }

    return 0;
}
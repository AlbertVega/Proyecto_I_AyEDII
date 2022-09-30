#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>

#include <opencv2/core.hpp>
#include <boost/serialization/split_free.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/stitching.hpp"

#include <string>
#include <vector>
using namespace std;
using namespace cv;

class ImageProcessing{
    private: //atributos
        int width;
        int height;
        int delta_bright;
        int ImageSource;
        int ImageDest;
        int gamma;
        Mat source;
        Mat destiny;
    public:
        ImageProcessing(int, int, int, int, int, int, Mat, Mat); //constructor
        int glaussian_blur();
        int gray_scale();
        int bright_control();
        int gamma_correction();

};
//constructor
ImageProcessing::ImageProcessing(int _witdh, int _height, int _delta_bright, int _ImageSource, int _ImageDest, int _gamma, Mat _source, Mat _destiny) {
    width = _witdh;
    height = _height;
    delta_bright = _delta_bright;
    ImageSource = _ImageSource;
    ImageDest = _ImageDest;
    gamma = _gamma;
    source = _source;
    destiny = _destiny;
}

int ImageProcessing::glaussian_blur() {
    GaussianBlur(source, destiny , Size(3, 3), 0);
    return 0;
}

int ImageProcessing::gray_scale() {
    cvtColor(source, destiny, COLOR_BGR2GRAY);
    return 0;
}
int ImageProcessing::bright_control() {
    source.convertTo(destiny, -1, 1, delta_bright);
}
int ImageProcessing::gamma_correction() {
    float invGamma = 1 / gamma;
    Mat table(1, 256, CV_8U);
    uchar *p = table.ptr();
    for (int i = 0; i < 256; ++i) {
        p[i] = (uchar) (pow(i / 255.0, invGamma) * 255);
    }
    LUT(source, table, destiny);
}

/**
 * function that serialize the image, convert the Mat image to serialized string and viceversa
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
    }
}
/**
 * function that convert the Mat image to serialized string
 * @param mat
 * @return
 */
std::string save( const cv::Mat & mat )
{
    std::ostringstream oss;
    boost::archive::text_oarchive toa( oss );
    toa << mat;

    return oss.str();
}

/**
 * function that convert serialized string to Mat image
 * @param mat
 * @param data_str
 */
void load( cv::Mat & mat, const char * data_str )
{
    std::stringstream ss;
    ss << data_str;

    boost::archive::text_iarchive tia( ss );
    tia >> mat;
}

/**
 * function that read the input messages
 * @param socket
 * @return
 */
string ReadMessage(boost::asio::ip::tcp::socket & socket) {
    boost::asio::streambuf buf;
    boost::asio::read_until( socket, buf, "\n" );
    string data = boost::asio::buffer_cast<const char*>(buf.data());
    return data;
}
/**
 * function that send output messages
 * @param socket
 * @param message
 */
void SendMessage(boost::asio::ip::tcp::socket & socket, string message) {
    string msg = message + "\n";
    boost::asio::write( socket, boost::asio::buffer(msg));
}

int main() {
    boost::asio::io_service io_service; //input/output service
    boost::asio::ip::tcp::acceptor acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),1234));
    boost::asio::ip::tcp::socket socket_(io_service); //declaration of socket

    cout << "Servidor iniciado" << endl;
    acceptor_.accept(socket_);
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
        blocks.push_back(result);
    }
    Mat Result;
    hconcat(blocks, Result);
    imshow("Result window", Result);
    waitKey(0);

    /**
     * create directory to see the result blocks of image
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
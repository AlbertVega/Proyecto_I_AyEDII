#include <iostream>
#include <boost/asio.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>

#include <opencv2/core.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <cassert>
#include <string>
#include <vector>

using namespace cv;
using namespace std;
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
 * function that divide the Mat image in several Mat objects
 * @param img
 * @param blockWidth
 * @param blocks
 * @return
 */
int divideImage(const cv::Mat& img, const int blockWidth, std::vector<cv::Mat>& blocks)
{
    // Checking if the image was passed correctly
    if (!img.data || img.empty())
    {
        std::cout << "Image Error: Cannot load image to divide." << std::endl;
        return EXIT_FAILURE;
    }

    // init image dimensions
    int imgWidth = img.cols;
    int imgHeight = img.rows;
    std::cout << "IMAGE SIZE: " << "(" << imgWidth << "," << imgHeight << ")" << std::endl;

    // init block dimensions
    int bwSize;
    int bhSize = img.rows;

    int y0 = 0;
    int x0 = 0;
    while (x0 < imgWidth)
    {
        bwSize = ((x0 + blockWidth) > imgWidth) * (blockWidth - (x0 + blockWidth - imgWidth)) + ((x0 + blockWidth) <= imgWidth) * blockWidth;

        blocks.push_back(img(cv::Rect(x0, y0, bwSize, bhSize)).clone());

        x0 = x0 + blockWidth;

    }
    return EXIT_SUCCESS;
}
/**
 * function that read the input messages
 * @param socket
 * @return
 */
string ReadMessage(boost::asio::ip::tcp::socket & socket) {
    boost::asio::streambuf buf; // Buffer de entrada de mensajes
    boost::asio::read_until( socket, buf, "\n"); //  Indica que lea mensaje del socket desde el buffer hasta el delimitador \n
    string data = boost::asio::buffer_cast<const char*>(buf.data()); // Hace cast del buffer de entrada a un char pointer (caracteres legibles)
    return data; // Retorna el mensaje recibido
}
/**
 * function that send output messages
 * @param socket
 * @param message
 */
void SendMessage(boost::asio::ip::tcp::socket & socket, string message) {
    string msg = message + "\n"; // Declara variable string con un delimitador linea siguiente
    boost::asio::write( socket, boost::asio::buffer(msg)); // Envia mensaje a cliente mediante buffer
}

int main() {
    /*
     * Image management
     */
    cv::Mat image = imread("/home/albert/Documentos/GitHub/Proyecto_I_AyEDII/src/Prueba.jpeg", IMREAD_COLOR);
    if (image.empty()) { //Verify if the image has been readed correctly
        cout << "Image File "
             << "Not Found" << endl;
        cin.get(); // wait for any key press
        return -1;
    }

    /*
     * Divide the image
     */
    const int blockw = 50; //size of the blocks image that will be generated
    std::vector<cv::Mat> blocks; //vector that contains the blocks of image
    int divideStatus = divideImage(image, blockw, blocks); //divide image

    /*
     * Socket
     */
    boost::asio::io_service io_service; //input/output service
    boost::asio::ip::tcp::socket socket(io_service); //declaration of sockets for conections

    socket.connect(boost::asio::ip::tcp::endpoint( boost::asio::ip::address::from_string("127.0.0.1"), 1234));
    cout << "Conectado al servidor" << endl;
    string size = to_string(blocks.size());
    /*
     * send the size of vector blocks
     */
    SendMessage(socket, size);
    string receivedMessage = ReadMessage(socket);
    receivedMessage.pop_back();
    cout << "Server dice que: "<<receivedMessage<<endl;

    /*
     * send the image source
     */
    for (int i = 0; i < blocks.size() ; i++){
        cv::Mat TEMP = blocks[i];
        std::string serialized = save(TEMP);
        SendMessage(socket, serialized);
        string receivedStatus = ReadMessage(socket);
        receivedStatus.pop_back();
        cout << "Server dice: "<<receivedStatus<<endl;
    }

    /*
     * receiving GammaApplied
     */
    vector<Mat>GammaBlocks;
    for (int i = 0; i < blocks.size(); i++) {
        string message = ReadMessage(socket); // Lee y declara mensaje del cliente
        SendMessage(socket, "Pedazo " + to_string(i) + " de Gamma recibido");
        message.pop_back();
        Mat result;
        load(result, message.c_str());
        GammaBlocks.push_back(result);
    }
    Mat ResultGammaApplied;
    hconcat(GammaBlocks, ResultGammaApplied);
    imshow("Image with Gamma_correction applied", ResultGammaApplied);
    waitKey(0);

    /*
     * receiving GrayApplied
     */
    vector<Mat>GrayBlocks;
    for (int i = 0; i < blocks.size(); i++) {
        string message = ReadMessage(socket); // Lee y declara mensaje del cliente
        SendMessage(socket, "Pedazo " + to_string(i) + " de Gray recibido");
        message.pop_back();
        Mat result;
        load(result, message.c_str());
        GrayBlocks.push_back(result);
    }
    Mat ResultGrayApplyied;
    hconcat(GrayBlocks, ResultGrayApplyied);
    imshow("Image with Gray_scale applied", ResultGrayApplyied);
    waitKey(0);

    /*
     * receiving BlurApplied
     */
    vector<Mat>BlurBlocks;
    for (int i = 0; i < blocks.size(); i++) {
        string message = ReadMessage(socket); // Lee y declara mensaje del cliente
        SendMessage(socket, "Pedazo " + to_string(i) + " de Blur recibido");
        message.pop_back();
        Mat result;
        load(result, message.c_str());
        BlurBlocks.push_back(result);
    }
    Mat ResultBlurApplyied;
    hconcat(BlurBlocks, ResultBlurApplyied);
    imshow("Image with gaussian_blur applyied", ResultBlurApplyied);
    waitKey(0);

    /*
     * receiving BrightApplied
     */
    vector<Mat>BrightBlocks;
    for (int i = 0; i < blocks.size(); i++) {
        string message = ReadMessage(socket); // Lee y declara mensaje del cliente
        SendMessage(socket, "Pedazo " + to_string(i) + " de Bright recibido");
        message.pop_back();
        Mat result;
        load(result, message.c_str());
        BrightBlocks.push_back(result);
    }
    Mat ResultBrightApplyied;
    hconcat(BrightBlocks, ResultBrightApplyied);
    imshow("Image with bright_control applyied", ResultBrightApplyied);
    waitKey(0);

    return 0;
}
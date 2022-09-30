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
#include <utility>
#include <vector>
using namespace std;
using namespace cv;
/**
 * class ImageProcessing
 */
class ImageProcessing{
    private: //atributes
        int width;
        int height;
        int delta_bright;
        float gamma;
        Mat *source;
        Mat *destiny;
    public:
        ImageProcessing(int, int, int, float, Mat*, Mat*); //constructor
        int gaussian_blur();
        int gray_scale();
        int bright_control();
        int gamma_correction();
};
//constructor
ImageProcessing::ImageProcessing(int _witdh, int _height, int _delta_bright, float _gamma, Mat *_source, Mat *_destiny) {
    width = _witdh;
    height = _height;
    delta_bright = _delta_bright;
    gamma = _gamma;
    source = _source;
    destiny = _destiny;
}
int ImageProcessing::gaussian_blur() {
    if (source->empty()){
        return -1;
    }else{
        GaussianBlur(*source, *destiny , Size(3, 3), 0);
        return 0;
    }
}
int ImageProcessing::gray_scale() {
    if (source->empty()){
        return -1;
    }else{
        cvtColor(*source, *destiny, COLOR_BGR2GRAY);
        return 0;
    }
}
int ImageProcessing::bright_control() {
    if (source->empty()){
        return -1;
    }else{
        source->convertTo(*destiny, -1, 1, delta_bright);
        return 0;
    }
}
int ImageProcessing::gamma_correction() {
    if (source->empty()){
        return -1;
    }else{
        float invGamma = 1 / gamma;
        Mat table(1, 256, CV_8U);
        uchar *p = table.ptr();
        for (int i = 0; i < 256; ++i) {
            p[i] = (uchar) (pow(i / 255.0, invGamma) * 255);
        }
        LUT(*source, table, *destiny);
        return 0;
    }
}
/**
 * Function that divide the image in blocks of Mat
 * @param img
 * @param blockWidth
 * @param blocks
 * @return
 */
int divideImage(const cv::Mat& img, const int blockWidth, std::vector<cv::Mat>& blocks){
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
    /*
     * receiving the image source
     */
    for (int i = 0; i < size; i++) {
        string message = ReadMessage(socket_); // Lee y declara mensaje del cliente
        SendMessage(socket_, "Pedazo " + to_string(i) + " recibido");
        message.pop_back();
        Mat result;
        load(result, message.c_str());
        blocks.push_back(result);
    }
    /*
     * create directory to see the result blocks of image
     */
    cv::utils::fs::createDirectory("Result");
    for (int j = 0; j < blocks.size(); j++)
    {
        std::string blockId = std::to_string(j);
        std::string blockImgName = "Result/block#" + blockId + ".jpeg";
        imwrite(blockImgName, blocks[j]);
    }
    Mat Result;
    hconcat(blocks, Result);
    /*
     * Applying filters
     */
    Mat GammaApplyed;
    ImageProcessing ImageGamma = ImageProcessing(Result.cols, Result.rows,75,3.0, &Result, &GammaApplyed);
    ImageGamma.gamma_correction();
    Mat BlurApplyed;
    ImageProcessing ImageBlur = ImageProcessing(Result.cols, Result.rows,100,3.0, &Result, &BlurApplyed);
    ImageBlur.gaussian_blur();
    Mat BrightApplyed;
    ImageProcessing ImageBright = ImageProcessing(Result.cols, Result.rows,100,3.0, &Result, &BrightApplyed);
    ImageBright.bright_control();
    Mat GrayApplyed;
    ImageProcessing ImageGray= ImageProcessing(Result.cols, Result.rows,100,3.0, &Result, &GrayApplyed);
    ImageGray.gray_scale();

    const int blockw = 50; //size of blocks

    /*
     * sending GammaApplyed
     */
    vector<Mat> blocksGamma;
    int divideGammaStatus = divideImage(GammaApplyed, blockw, blocksGamma); //divide image

    for (int i = 0; i < size ; i++){
        cv::Mat TEMP = blocksGamma[i];
        std::string serialized = save(TEMP);
        SendMessage(socket_, serialized);
        string receivedStatus = ReadMessage(socket_);
        receivedStatus.pop_back();
        cout << "Cliente dice: "<<receivedStatus<<endl;
    }

    /*
     * sending GrayAppied
     */
    vector<Mat> blocksGray;
    int divideGrayStatus = divideImage(GrayApplyed, blockw, blocksGray); //divide image

    for (int i = 0; i < size ; i++){
        cv::Mat TEMP = blocksGray[i];
        std::string serialized = save(TEMP);
        SendMessage(socket_, serialized);
        string receivedStatus = ReadMessage(socket_);
        receivedStatus.pop_back();
        cout << "Cliente dice: "<<receivedStatus<<endl;
    }

    /*
     * sending BlurApplied
     */
    vector<Mat> blocksBlur;
    int divideBlurStatus = divideImage(BlurApplyed, blockw, blocksBlur); //divide image

    for (int i = 0; i < size ; i++){
        cv::Mat TEMP = blocksBlur[i];
        std::string serialized = save(TEMP);
        SendMessage(socket_, serialized);
        string receivedStatus = ReadMessage(socket_);
        receivedStatus.pop_back();
        cout << "Cliente dice: "<<receivedStatus<<endl;
    }

    /*
     * sending BrightApplied
     */
    vector<Mat> blocksBright;
    int divideBrightStatus = divideImage(BrightApplyed, blockw, blocksBright); //divide image
    for (int i = 0; i < size ; i++){
        cv::Mat TEMP = blocksBright[i];
        std::string serialized = save(TEMP);
        SendMessage(socket_, serialized);
        string receivedStatus = ReadMessage(socket_);
        receivedStatus.pop_back();
        cout << "Cliente dice: "<<receivedStatus<<endl;
    }
    return 0;
}
#include <iostream>
#include <boost/asio.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>


using namespace cv;
using namespace std;

/*
 * image divider
 */
int divideImage(const cv::Mat& img, const int blockWidth, const int blockHeight, std::vector<cv::Mat>& blocks)
{
    // Checking if the image was passed correctly
    if (!img.data || img.empty())
    {
        std::wcout << "Image Error: Cannot load image to divide." << std::endl;
        return EXIT_FAILURE;
    }

    // init image dimensions
    int imgWidth = img.cols;
    int imgHeight = img.rows;
    std::wcout << "IMAGE SIZE: " << "(" << imgWidth << "," << imgHeight << ")" << std::endl;

    // init block dimensions
    int bwSize;
    int bhSize;

    int y0 = 0;
    while (y0 < imgHeight)
    {
        // compute the block height
        bhSize = ((y0 + blockHeight) > imgHeight) * (blockHeight - (y0 + blockHeight - imgHeight)) + ((y0 + blockHeight) <= imgHeight) * blockHeight;

        int x0 = 0;
        while (x0 < imgWidth)
        {
            // compute the block height
            bwSize = ((x0 + blockWidth) > imgWidth) * (blockWidth - (x0 + blockWidth - imgWidth)) + ((x0 + blockWidth) <= imgWidth) * blockWidth;

            // crop block
            blocks.push_back(img(cv::Rect(x0, y0, bwSize, bhSize)).clone());

            // update x-coordinate
            x0 = x0 + blockWidth;
        }

        // update y-coordinate
        y0 = y0 + blockHeight;
    }
    return EXIT_SUCCESS;
}

int main() {
    /*
     * Image management
     */
    cv::Mat image = imread("/home/albert/Documentos/GitHub/Proyecto_I_AyEDII/Prueba.jpeg", IMREAD_COLOR);

    if (image.empty()) { //Verify if the image has been readed correctly
        cout << "Image File "
             << "Not Found" << endl;
        cin.get(); // wait for any key press
        return -1;
    }
    imshow("ImageWindow", image);
    waitKey(0);

    /*
     * Divide the image
     */
    // init vars
    const int blockw = 128;
    const int blockh = 128;
    std::vector<cv::Mat> blocks;
    int divideStatus = divideImage(image, blockw, blockh, blocks);

    // debug: save blocks
    cv::utils::fs::createDirectory("blocksFolder");
    for (int j = 0; j < blocks.size(); j++)
    {
        std::string blockId = std::to_string(j);
        std::string blockImgName = "blocksFolder/block#" + blockId + ".jpeg";
        imwrite(blockImgName, blocks[j]);
    }

    /*
     * Socket
     */
    boost::asio::io_service io_service; // Servicio de input/output
    boost::asio::ip::tcp::socket socket(io_service); // Declaracion de socket para conexiones
    boost::system::error_code error; // Variable para codigo de error especifico de Boost
    boost::asio::streambuf receive_buffer; // Buffer para recibir mensajes
    const string outMessage = "Hola, soy el cliente\n"; // Mensaje a enviar

    socket.connect(boost::asio::ip::tcp::endpoint( boost::asio::ip::address::from_string("127.0.0.1"), 1234));
    cout << "Conectado al servidor" << endl;
    boost::asio::write(socket, boost::asio::buffer(outMessage), error );

    // Escribe mensaje al servidor

    if(!error) {
        cout << "Mensaje enviado" << endl;
    }
    else {
        cout << "error" << error.message() << endl;
    }

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
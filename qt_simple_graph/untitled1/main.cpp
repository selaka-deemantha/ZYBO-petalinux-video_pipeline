#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <vector>
#include <complex>
#include <algorithm>

#define DEVICE_PATH "/dev/fft_dma"
#define NUM_SAMPLES 1024

class FFTWidget : public QWidget {
    Q_OBJECT
public:
    FFTWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(640, 480);

        // Timer for continuous updates
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &FFTWidget::readAndUpdateFFT);
        timer->start(20); // ~50 FPS
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::black);

        if (spectrum.empty())
            return;

        int N = spectrum.size();
        float w = width() / float(N);
        float hScale = float(height()) / (*std::max_element(spectrum.begin(), spectrum.end()));

        painter.setPen(Qt::green);
        for (int i = 0; i < N; ++i) {
            float h = spectrum[i] * hScale;
            painter.drawLine(QPointF(i * w, height()), QPointF(i * w, height() - h));
        }
    }

private slots:
    void readAndUpdateFFT() {
        int fd = open(DEVICE_PATH, O_RDONLY);
        if (fd < 0) {
            perror("Cannot open /dev/fft_dma");
            return;
        }

        uint64_t buf[NUM_SAMPLES];
        ssize_t ret = read(fd, buf, NUM_SAMPLES * sizeof(uint64_t));
        ::close(fd);
        if (ret != NUM_SAMPLES * sizeof(uint64_t)) {
            fprintf(stderr, "Failed to read all FFT samples\n");
            return;
        }

        // Parse as complex numbers
        std::vector<std::complex<float>> data;
        data.reserve(NUM_SAMPLES);
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            uint64_t val = buf[i];
            int32_t real = int32_t((val >> 32) & 0xFFFFFFFF);
            int32_t imag = int32_t(val & 0xFFFFFFFF);
            data.push_back(std::complex<float>(real, imag));
        }

        // Compute magnitude (positive frequencies only)
        spectrum.clear();
        for (int i = 0; i < NUM_SAMPLES / 2; ++i)
            spectrum.push_back(std::abs(data[i]));

        update(); // trigger repaint
    }

private:
    std::vector<float> spectrum;
};

int main(int argc, char *argv[])
{
    // Set framebuffer environment (LinuxFB)
    qputenv("QT_QPA_PLATFORM", QByteArray("linuxfb"));
    qputenv("QT_QPA_FB", QByteArray("/dev/fb0"));

    QApplication app(argc, argv);

    FFTWidget w;
    w.showFullScreen(); // important for LinuxFB

    return app.exec();
}

#include "main.moc"

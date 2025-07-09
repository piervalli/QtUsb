#ifndef QUSBDEVICEINFO_H
#define QUSBDEVICEINFO_H

#include <QObject>
#include "qobjectdefs.h"
#include "qusbglobal.h"

QT_BEGIN_NAMESPACE


/*!
 * xusb.c
 * \brief The QUsbDeviceInfo class
 */
class Q_USB_EXPORT QUsbDeviceInfo : public QObject
{
    Q_OBJECT
public:
    explicit QUsbDeviceInfo(QObject *parent = nullptr);
    QString deviceInfo(quint16 vid, quint16 pid);

    enum DeviceClass : uint8_t {
        USB_CLASS_PER_INTERFACE          = 0x00,  // Use class info in Interface Descriptors
        USB_CLASS_AUDIO                  = 0x01,  // Audio devices
        USB_CLASS_COMM                   = 0x02,  // Communications and CDC Control
        USB_CLASS_HID                    = 0x03,  // Human Interface Device
        USB_CLASS_PHYSICAL               = 0x05,  // Physical Interface Device
        USB_CLASS_IMAGE                  = 0x06,  // Still Imaging (cameras)
        USB_CLASS_PRINTER                = 0x07,  // Printer devices
        USB_CLASS_MASS_STORAGE           = 0x08,  // Mass Storage (USB drives, etc.)
        USB_CLASS_HUB                    = 0x09,  // USB Hub
        LIBUSB_CLASS_DATA                = 0x0A,  // CDC-Data
        USB_CLASS_SMART_CARD             = 0x0B,  // Smart Card
        USB_CLASS_CONTENT_SECURITY       = 0x0D,  // Content Security
        USB_CLASS_VIDEO                  = 0x0E,  // Video devices (UVC)
        USB_CLASS_PERSONAL_HEALTHCARE    = 0x0F,  // Personal Healthcare
        USB_CLASS_AUDIO_VIDEO            = 0x10,  // Audio/Video Devices
        USB_CLASS_BILLBOARD              = 0x11,  // Billboard Device Class
        USB_CLASS_USB_TYPE_C_BRIDGE      = 0x12,  // USB Type-C Bridge Class
        USB_CLASS_I3C                    = 0x3C,  // I3C Device Class
        USB_CLASS_DIAGNOSTIC             = 0xDC,  // Diagnostic Device
        USB_CLASS_WIRELESS_CONTROLLER    = 0xE0,  // Wireless Controller (Bluetooth, WiFi)
        USB_CLASS_MISCELLANEOUS          = 0xEF,  // Miscellaneous
        USB_CLASS_APPLICATION_SPECIFIC   = 0xFE,  // Application Specific
        USB_CLASS_VENDOR_SPECIFIC        = 0xFF   // Vendor Specific
    };
    Q_ENUM(DeviceClass)

    struct Q_USB_EXPORT EndpointInfo {
        uint8_t interface_number;
        uint8_t endpoint_in;        // bEndpointAddress IN
        uint8_t endpoint_out;       // bEndpointAddress OUT
        uint16_t max_packet_in;     // wMaxPacketSize IN
        uint16_t max_packet_out;    // wMaxPacketSize OUT
        uint8_t bmAttributes_in;    // bmAttributes IN endpoint
        uint8_t bmAttributes_out;   // bmAttributes OUT endpoint
        uint8_t interface_class;
        uint8_t interface_subclass;
        uint8_t interface_protocol;
    };

    class Q_USB_EXPORT IdInfo
    {
    public:
        IdInfo();
        IdInfo(const QUsbDeviceInfo::IdInfo &other);
        bool operator==(const QUsbDeviceInfo::IdInfo &other) const;
        QUsbDeviceInfo::IdInfo &operator=(QUsbDeviceInfo::IdInfo other);
        operator QString() const;

        quint16 pid;
        quint16 vid;
        quint8 bus;
        quint8 port;
        quint8 dClass;
        quint8 dSubClass;
        QString productName;
        QList<EndpointInfo> endpoints;

    };

    typedef QList<IdInfo> IdInfoList;

    static IdInfoList devices(QList<QUsbDeviceInfo::DeviceClass> filter= QList<QUsbDeviceInfo::DeviceClass>{},bool allInfo=false);

protected:
    QString info(quint16 vid, quint16 pid);
signals:
};
typedef QUsbDeviceInfo::DeviceClass QUsbDeviceInfoDeviceClass;
Q_DECLARE_METATYPE(QUsbDeviceInfoDeviceClass)
// Q_DECLARE_METATYPE(QUsbDeviceInfo::IdInfo)
QT_END_NAMESPACE

#endif // QUSBDEVICEINFO_H

#include "qusbdeviceinfo.h"
#include "libusb.h"
#include <QDebug>
#if defined(Q_OS_MACOS)
#include <libusb.h>
#include <hidapi.h>
#elif defined(Q_OS_ANDROID)
#include <libusb.h>
#include <hidapi.h>
#elif defined(Q_OS_UNIX)
#include <libusb-1.0/libusb.h>
#include <hidapi.h>
#else
#include <libusb/libusb.h>
#include <hidapi/hidapi.h>
#endif
#if  defined(Q_OS_WIN)
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#endif
#include <QRegularExpression>

QUsbDeviceInfo::QUsbDeviceInfo(QObject *parent)
    : QObject { parent }

{
    qRegisterMetaType<QUsbDeviceInfo::IdInfo>("QUsbDeviceInfo::IdInfo");
    qRegisterMetaType<QUsbDeviceInfo::IdInfoList>("QUsbDeviceInfo::IdInfoList");
    qRegisterMetaType<QUsbDeviceInfo::DeviceClass>("QUsbDeviceInfo::DeviceClass");
}

QString QUsbDeviceInfo::deviceInfo(quint16 vid, quint16 pid)
{
    QString buffer;
    const struct libusb_version *version;
    int r;
    version = libusb_get_version();
    buffer.append(QStringLiteral("Using libusb v%1.%2.%3.%4\n\n").arg(version->major).arg(version->minor).arg(version->micro).arg(version->nano));
#ifdef Q_OS_ANDROID
    libusb_set_option(NULL, LIBUSB_OPTION_NO_DEVICE_DISCOVERY, NULL);
#endif
    r = libusb_init(NULL);
    if (r < 0)
        return buffer;
    buffer.append(info(vid, pid));
    libusb_exit(NULL);
    return buffer;
}

QUsbDeviceInfo::IdInfoList QUsbDeviceInfo::devices(QList<DeviceClass> filter, bool allInfo)
{
    QUsbDeviceInfo::IdInfoList list;
    ssize_t cnt; // holding number of devices in list
    libusb_device **devs;
    libusb_context *ctx;
    struct hid_device_info *hid_devs, *cur_hid_dev;

#ifdef Q_OS_ANDROID
    libusb_set_option(NULL, LIBUSB_OPTION_NO_DEVICE_DISCOVERY, NULL);
#endif
    libusb_init(&ctx);
    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_NONE);
    cnt = libusb_get_device_list(ctx, &devs); // get the list of devices
    if (cnt < 0) {
        qCritical("libusb_get_device_list Error");
        libusb_free_device_list(devs, 1);
        return list;
    }
    auto match_vid_pid = [](const QString& hardware_id, quint16& avid, quint16& apid){
        quint16 vid = 0;
        quint16 pid = 0;
        int idx = hardware_id.indexOf(QStringLiteral("VID_"));
        if (idx != -1) {
            QString vidStr = hardware_id.mid(idx + 4, 4); // 4 hex digits
            vid = vidStr.toUShort(nullptr, 16);
        }

        idx = hardware_id.indexOf(QStringLiteral("PID_"));
        if (idx != -1) {
            QString pidStr = hardware_id.mid(idx + 4, 4); // 4 hex digits
            pid = pidStr.toUShort(nullptr, 16);
        }

        // qCritical() << hardware_id << "vid" << avid << vid << "pid"<<apid <<  pid;
        return avid ==vid && apid==pid; // No match found
    };
    for (int i = 0; i < cnt; i++)
    {
        libusb_device *dev = devs[i];
        libusb_device_descriptor desc;

        if (libusb_get_device_descriptor(dev, &desc) == 0)
        {


            QUsbDeviceInfo::IdInfo id;
            id.pid = desc.idProduct;
            id.vid = desc.idVendor;
            id.bus = libusb_get_bus_number(dev);
            id.port = libusb_get_port_number(dev);
            id.dClass = desc.bDeviceClass;
            id.dSubClass = desc.bDeviceSubClass;
            const auto classEnum=static_cast<QUsbDeviceInfo::DeviceClass>(desc.bDeviceClass);
            // qCritical() << "filter" << desc.bDeviceClass << desc.bDeviceSubClass  << "productName" << id.productName << allInfo;
            if(!filter.isEmpty() && !filter.contains(classEnum))
            {
                continue;
            }
            libusb_config_descriptor *config_desc;
            int result = libusb_get_active_config_descriptor(dev, &config_desc);
            if (result ==LIBUSB_SUCCESS)
            {
                bool has_transfer_bulk_interface_in = false;
                bool has_transfer_bulk_interface_out = false;

                       // Scansiona tutte le interfacce
                for (int intf = 0; intf < config_desc->bNumInterfaces; intf++)
                {
                    const libusb_interface *interface = &config_desc->interface[intf];

                           // Controlla tutte le alternate settings
                    for (int alt = 0; alt < interface->num_altsetting; alt++)
                    {
                        const libusb_interface_descriptor *interface_desc = &interface->altsetting[alt];


                        EndpointInfo endpoint_info{};
                        //start endpoint

                        endpoint_info.interface_number = interface_desc->bInterfaceNumber;
                        endpoint_info.interface_class = interface_desc->bInterfaceClass;
                        endpoint_info.interface_subclass = interface_desc->bInterfaceSubClass;
                        endpoint_info.interface_protocol = interface_desc->bInterfaceProtocol;

                        qInfo() << "    Interface " << (int)interface_desc->bInterfaceNumber
                                << " (Class: " << (int)interface_desc->bInterfaceClass
                                << ", SubClass: " << (int)interface_desc->bInterfaceSubClass
                                << ", Protocol: " << (int)interface_desc->bInterfaceProtocol << ")";

                               // Analizza gli endpoint
                        for (int ep = 0; ep < interface_desc->bNumEndpoints; ep++)
                        {
                            const libusb_endpoint_descriptor *endpoint = &interface_desc->endpoint[ep];
                            uint8_t endpoint_addr = endpoint->bEndpointAddress;
                            uint8_t transfer_type = endpoint->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK;



                            if (endpoint_addr & LIBUSB_ENDPOINT_IN) {

                                endpoint_info.endpoint_in = endpoint_addr;
                                endpoint_info.max_packet_in = endpoint->wMaxPacketSize;
                                endpoint_info.bmAttributes_in = endpoint->bmAttributes;

                                qInfo() << " (IN)  " << endpoint_info.endpoint_in
                                        << "max_packet 0x" << QString::number(endpoint_info.max_packet_in, 16)
                                        << "type" << endpoint_info.bmAttributes_in;
                                if (transfer_type == LIBUSB_TRANSFER_TYPE_BULK) {
                                    has_transfer_bulk_interface_in=true;
                                } else if (transfer_type == LIBUSB_TRANSFER_TYPE_INTERRUPT) {

                                } else if (transfer_type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {

                                }

                            } else {

                                endpoint_info.endpoint_out = endpoint_addr;
                                endpoint_info.max_packet_out = endpoint->wMaxPacketSize;
                                endpoint_info.bmAttributes_out = endpoint->bmAttributes;

                                qInfo() << " (OUT)  " << endpoint_info.endpoint_out
                                        << "max_packet 0x" << QString::number(endpoint_info.max_packet_out, 16)
                                        << "type" << endpoint_info.bmAttributes_out;

                                if (transfer_type == LIBUSB_TRANSFER_TYPE_BULK) {
                                    has_transfer_bulk_interface_out=true;
                                } else if (transfer_type == LIBUSB_TRANSFER_TYPE_INTERRUPT) {

                                } else if (transfer_type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {

                                }

                            }

                        }

                               //end endpoint
                        if(has_transfer_bulk_interface_in && has_transfer_bulk_interface_out) {
                            id.endpoints.push_back(endpoint_info);
                        }

                    }
                }

                libusb_free_config_descriptor(config_desc);
            }
            if(allInfo)
            {

                // Get string descriptors
                libusb_device_handle *handle;
                if (libusb_open(dev, &handle) == 0)
                {
                    int ret;

                           // Get manufacturer string
                    if (desc.iManufacturer > 0) {
                        unsigned char manufacturer[256] = {0};
                        ret = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer,
                                                                 manufacturer, sizeof(manufacturer));
                        if (ret > 0) {
                            id.productName.append(QString::fromUtf8((char*)manufacturer));
                            id.productName.append(' ');
                        }
                    }

                           // Get product string
                    if (desc.iProduct > 0) {
                        unsigned char product[256] = {0};
                        ret = libusb_get_string_descriptor_ascii(handle, desc.iProduct,
                                                                 product, sizeof(product));
                        if (ret > 0) {
                            id.productName.append(QString::fromUtf8((char*)product));
                        }
                    }

                    libusb_close(handle);
                }else {
                    id.productName.append(QStringLiteral("DEVICE USB"));
#if defined(Q_OS_WIN)

                    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(
                            NULL,  // All device classes
                            L"USB", // USB enumerator
                            NULL,
                            DIGCF_PRESENT | DIGCF_ALLCLASSES
                            );

                    if (deviceInfoSet == INVALID_HANDLE_VALUE)
                    {
                        qCritical() << "SetupDiGetClassDevs failed: " << GetLastError();
                    }else
                    {

                        SP_DEVINFO_DATA deviceInfoData;
                        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

                        for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++)
                        {

                            char buffer[512];
                            DWORD bufferSize;

                                   // Get Hardware ID
                            bufferSize = sizeof(buffer);
                            if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData,
                                                                  SPDRP_HARDWAREID, NULL,
                                                                  (BYTE*)buffer, bufferSize, NULL)) {
                                std::string hardware_id(buffer);

                                if(match_vid_pid(QString::fromStdString(hardware_id),id.vid,id.pid))
                                {

                                    auto bufferSize = sizeof(buffer);
                                    if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData,
                                                                          SPDRP_DEVICEDESC, NULL,
                                                                          (BYTE*)buffer, bufferSize, NULL))
                                    {
                                        auto temp = QString::fromStdString(std::string(buffer)).split(QStringLiteral("\\"));
                                        id.productName.clear();
                                        id.productName.append(temp.first());
                                        break;
                                    }
                                }

                            }
                        }

                        SetupDiDestroyDeviceInfoList(deviceInfoSet);
                    }

#endif
                }
            }
            // qCritical() << "filter" << desc.bDeviceClass << desc.bDeviceSubClass  << "productName" << id.productName << allInfo;
            list.append(id);
        }
    }

    libusb_free_device_list(devs, 1);
    libusb_exit(ctx);

    hid_devs = hid_enumerate(0x0, 0x0);
    cur_hid_dev = hid_devs;
    while (cur_hid_dev) {

        QUsbDeviceInfo::IdInfo id;
        id.pid = cur_hid_dev->product_id;
        id.vid = cur_hid_dev->vendor_id;
        id.bus = 0;
        id.port = 0;

        list.append(id);

        cur_hid_dev = cur_hid_dev->next;
    }
    hid_free_enumeration(hid_devs);

    return list;
}



QString QUsbDeviceInfo::info(quint16 vid, quint16 pid)
{
    QString buffer;
#ifdef Q_OS_ANDROID
    // On Android with NO_DEVICE_DISCOVERY, libusb_open_device_with_vid_pid cannot enumerate devices
    // Device access must be done through Android UsbManager and file descriptors via JNI
    buffer.append(QStringLiteral("Device info not available on Android.\n"));
    buffer.append(QStringLiteral("Use Android UsbManager to access devices.\n"));
    buffer.append(QStringLiteral("Requested VID:PID = %1:%2\n").arg(QString::number(vid, 16), QString::number(pid, 16)));
    return buffer;
#else
    libusb_device_handle *handle;
    libusb_device *dev;
    uint8_t bus, port_path[8];

    int i,r;

    const char *const speed_name[6] = { "Unknown", "1.5 Mbit/s (USB LowSpeed)", "12 Mbit/s (USB FullSpeed)",
                                        "480 Mbit/s (USB HighSpeed)", "5000 Mbit/s (USB SuperSpeed)", "10000 Mbit/s (USB SuperSpeedPlus)" };

    buffer.append(QStringLiteral("Opening device %1:%2\n").arg(QString::number(vid, 16),QString::number(pid, 16)));
    handle = libusb_open_device_with_vid_pid(NULL, vid, pid);

    if (handle == NULL) {
        buffer.append(QStringLiteral("  Failed.\n"));
        return buffer;
    }
    dev = libusb_get_device(handle);
    bus = libusb_get_bus_number(dev);
    bool extra_info = true; //info
    if (extra_info) {
        r = libusb_get_port_numbers(dev, port_path, sizeof(port_path));
        if (r > 0) {
            buffer.append(QStringLiteral("\nDevice properties:\n"));
            buffer.append(QStringLiteral("        bus number: %1\n").arg(bus));
            buffer.append(QStringLiteral("         port path: %1").arg(port_path[0]));
            for (i = 1; i < r; i++) {
                printf("->%d", port_path[i]);
            }
            buffer.append(QStringLiteral(" (from root hub)\n"));
        }
        r = libusb_get_device_speed(dev);
        if ((r < 0) || (r > 5))
            r = 0;
        buffer.append(QStringLiteral("             speed: %1\n").arg(QString::fromUtf8(speed_name[r])));
    }
    return buffer;
#endif
}

QUsbDeviceInfo::IdInfo::IdInfo()
{

}

QUsbDeviceInfo::IdInfo::IdInfo(const IdInfo &other)
    :pid(other.pid), vid(other.vid), bus(other.bus), port(other.port), dClass(other.dClass), dSubClass(other.dSubClass),productName(other.productName),endpoints(other.endpoints)
{

}

bool QUsbDeviceInfo::IdInfo::operator==(const IdInfo &other) const
{
    return other.pid == pid
            && other.vid == vid
            && other.bus == bus
            && other.port == port
            && other.dClass == dClass
            && other.dSubClass == dSubClass;
}

QUsbDeviceInfo::IdInfo &QUsbDeviceInfo::IdInfo::operator=(IdInfo other)
{
    pid = other.pid;
    vid = other.vid;
    bus = other.bus;
    port = other.port;
    dClass = other.dClass;
    dSubClass = other.dSubClass;
    productName=other.productName;
    return *this;
}

QUsbDeviceInfo::IdInfo::operator QString() const
{
    return QString::fromUtf8("Id(Vid: %1, Pid: %2, Bus: %3, Port: %4, Class: %5, Subclass: %6)")
    .arg(vid, 4, 16, QChar::fromLatin1('0'))
            .arg(pid, 4, 16, QChar::fromLatin1('0'))
            .arg(bus)
            .arg(port)
            .arg(dClass)
            .arg(dSubClass);
}

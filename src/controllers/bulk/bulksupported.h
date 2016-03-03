/**
* @file bulksupported.h
* @author Neale Picket  neale@woozle.org
* @date Thu Jun 28 2012
* @brief A list of supported USB bulk devices
*/

_Pragma("once")
struct bulk_supported_type {
    unsigned short vendor_id;
    unsigned short product_id;
    unsigned char in_epaddr;
    unsigned char out_epaddr;
};

static bulk_supported_type bulk_supported[] = {
    {0x06f8, 0xb105, 0x82, 0x03}, // Hercules MP3e2
    {0x06f8, 0xb107, 0x83, 0x03}, // Hercules Mk4
    {0x06f8, 0xb100, 0x86, 0x06}, // Hercules Mk2
    {0, 0, 0, 0}
};

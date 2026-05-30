#include "netget.h"
#include "../lightshell.h"
#include "../printf.h"
#include "../kmalloc.h"
#include "../kernel_util.h"
#include "../fs/ramfs.h"
#include "../../net/http/http.h"
#include "../../net/dns/dns.h"
#include "../../drivers/serial.h"

int cmd_netget_handler(const char *arg)
{
    if (!arg || arg[0] == '\0') {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "Usage: netget <url> [output-file]\n");
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Downloads URL content to file or displays it\n");
        return 1;
    }
    
    /* Simple argument parsing - find first space */
    const char *url = arg;
    const char *output_file = NULL;
    
    /* Find end of URL (first space) */
    int url_len = 0;
    while (arg[url_len] && arg[url_len] != ' ') {
        url_len++;
    }
    
    /* Check for output file argument */
    if (arg[url_len] == ' ' && arg[url_len + 1]) {
        output_file = &arg[url_len + 1];
    }
    
    /* Simple URL parsing - expecting http://host/path */
    if (kutil_strncmp(url, "http://", 7) != 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "Error: Only http:// URLs supported\n");
        return 1;
    }
    
    const char *host_start = url + 7;
    const char *path_start = kutil_strchr(host_start, '/');
    
    char host[256];
    char path[256];
    
    if (path_start == (const char *)0) {
        /* No path specified, just host */
        int copy_len = url_len - 7;
        if (copy_len >= (int)sizeof(host)) copy_len = sizeof(host) - 1;
        kutil_memcpy(host, host_start, copy_len);
        host[copy_len] = '\0';
        kutil_strcpy(path, "/");
    } else {
        /* Split host and path */
        int host_len = path_start - host_start;
        if (host_len >= (int)sizeof(host)) host_len = sizeof(host) - 1;
        kutil_memcpy(host, host_start, host_len);
        host[host_len] = '\0';
        
        kutil_strcpy(path, path_start);
    }
    
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "Fetching: http://");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, host);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, path);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    
    /* Allocate buffer for response */
    uint8_t *buffer = (uint8_t *)brights_kmalloc(8192); /* 8KB buffer */
    if (buffer == (uint8_t *)0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "Error: Out of memory\n");
        return 1;
    }
    
    uint32_t bytes_received = 0;
    int result = brights_http_get(host, path, buffer, 8192, &bytes_received);
    
    if (result != 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "Error: Failed to fetch URL\n");
        brights_kfree(buffer);
        return 1;
    }
    
    /* Display the response (skip HTTP headers for simplicity) */
    uint8_t *content_start = buffer;
    uint32_t content_len = bytes_received;
    
    /* Simple header/body separation (look for \r\n\r\n) */
    for (uint32_t i = 0; i < bytes_received - 3; i++) {
        if (buffer[i] == '\r' && buffer[i+1] == '\n' && 
            buffer[i+2] == '\r' && buffer[i+3] == '\n') {
            content_start = &buffer[i+4];
            content_len = bytes_received - (i+4);
            break;
        }
    }

    /* If output file specified, write to file */
    if (output_file != (const char *)0) {
        int fd = brights_ramfs_create(output_file);
        if (fd < 0) {
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, "Error: Cannot create file ");
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, output_file);
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
        } else {
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, "Saving to file: ");
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, output_file);
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, " (");
            
            char size_buf[32];
            kutil_utoa(content_len, size_buf, 10);
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, size_buf);
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, " bytes)\n");
            
            brights_ramfs_write(fd, 0, content_start, content_len);
            brights_ramfs_close(fd);
        }
    }
    
    /* Write content to serial */
    if (content_len > 0) {
        /* Write content byte by byte since we don't have a binary write function */
        for (uint32_t i = 0; i < content_len; i++) {
            char ch[2] = {(char)content_start[i], 0};
            brights_serial_write_ascii(BRIGHTS_COM1_PORT, ch);
        }
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    }
    
    brights_kfree(buffer);
    return 1;
}
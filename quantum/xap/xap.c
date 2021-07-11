/* Copyright 2021 QMK
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <quantum.h>
#include <xap.h>
#include <usb_descriptor.h>

typedef uint8_t  xap_identifier_t;
typedef uint16_t xap_token_t;

typedef enum xap_route_type_t {
    XAP_UNKNOWN = 0,
    XAP_ROUTE,
    XAP_EXECUTE,
} xap_route_type_t;

typedef struct xap_route_flags_t {
    xap_route_type_t type : 2;
    uint8_t          is_secure : 1;
} xap_route_flags_t;

_Static_assert(sizeof(xap_route_flags_t) == 1, "xap_route_flags_t is not length of 1");

extern void xap_send(uint8_t *data, uint8_t length);

#define XAP_SUBSYSTEM_XAP 0x00
#define XAP_SUBSYSTEM_XAP_ROUTE_VERSION 0x00

#define XAP_SUBSYSTEM_QMK 0x01
#define XAP_SUBSYSTEM_QMK_CAPABILITIES_QUERY 0x00
#define XAP_SUBSYSTEM_QMK_ROUTE_VERSION 0x01

#define XAP_SUBSYSTEM_KEYBOARD 0x02

#define XAP_SUBSYSTEM_USER 0x03

typedef struct xap_route_t xap_route_t;
struct xap_route_t {
    const xap_route_flags_t flags;
    union {
        struct {
            const xap_route_t *child_routes;
            const uint8_t      child_routes_len;
        };
        void (*handler)(xap_token_t token, const uint8_t *data, size_t data_len);
    };
};

#ifdef CONSOLE_ENABLE
#    define DUMP_XAP_DATA(name, token, data, len)                             \
        do {                                                                  \
            dprintf("%s(%04X, ..., %d):", (#name), (int)(token), (int)(len)); \
            for (int i = 0; i < (len); ++i) {                                 \
                dprintf(" %02X", (int)((data)[i]));                           \
            }                                                                 \
            dprint("\n");                                                     \
        } while (0)
#else
#    define DUMP_XAP_DATA(name, token, data, len) \
        do {                                      \
        } while (0)
#endif

void xap_route_version(xap_token_t token, const uint8_t *data, size_t data_len) {}
void qmk_route_version(xap_token_t token, const uint8_t *data, size_t data_len) {
    // DUMP_XAP_DATA(qmk_route_version, token, data, data_len);
    uint8_t rdata[XAP_EPSIZE] = {0};
    *(uint16_t *)rdata        = token;
    rdata[2]                  = 0x01;
    *(uint32_t *)&rdata[3]    = 0x12345678;
    xap_send(rdata, sizeof(rdata));
}
void qmk_caps_query(xap_token_t token, const uint8_t *data, size_t data_len) {}

static const xap_route_t xap_routes[] = {
    [XAP_SUBSYSTEM_XAP_ROUTE_VERSION] = {.flags = {.type = XAP_EXECUTE, .is_secure = 0}, .handler = &xap_route_version},
};

static const xap_route_t qmk_routes[] = {
    [XAP_SUBSYSTEM_QMK_CAPABILITIES_QUERY] = {.flags = {.type = XAP_EXECUTE, .is_secure = 0}, .handler = &qmk_caps_query},
    [XAP_SUBSYSTEM_QMK_ROUTE_VERSION]      = {.flags = {.type = XAP_EXECUTE, .is_secure = 0}, .handler = &qmk_route_version},
};

static const xap_route_t root_routes[] = {
    [XAP_SUBSYSTEM_XAP] = {.flags = {.type = XAP_ROUTE, .is_secure = 0}, .child_routes = xap_routes, .child_routes_len = sizeof(xap_routes) / sizeof(xap_routes[0])},
    [XAP_SUBSYSTEM_QMK] = {.flags = {.type = XAP_ROUTE, .is_secure = 0}, .child_routes = qmk_routes, .child_routes_len = sizeof(qmk_routes) / sizeof(qmk_routes[0])},
};

void xap_respond_failure(xap_token_t token) {
    uint8_t data[XAP_EPSIZE] = {0};
    *(uint16_t *)data        = token;
    data[2]                  = 0;
    xap_send(data, sizeof(data));
}

void xap_execute_route(xap_token_t token, const xap_route_t *routes, size_t max_routes, const uint8_t *data, size_t data_len) {
    // DUMP_XAP_DATA(xap_execute_route, token, data, data_len);
    xap_identifier_t id = data[0];
    if (id < max_routes) {
        const xap_route_t *route = &routes[id];
        switch (route->flags.type) {
            case XAP_ROUTE:
                if (route->child_routes != NULL && route->child_routes_len > 0) {
                    xap_execute_route(token, route->child_routes, route->child_routes_len, &data[1], data_len - 1);
                }
                break;
            case XAP_EXECUTE:
                if (route->handler != NULL) {
                    (route->handler)(token, &data[1], data_len - 1);
                }
                break;
            default:
                xap_respond_failure(token);
                break;
        }
    }
}

void xap_receive(const void *data, size_t length) {
    const uint8_t *u8data = (const uint8_t *)data;
    xap_token_t    token  = *(const xap_token_t *)data;
    // DUMP_XAP_DATA(xap_receive, token, u8data, length);
    xap_execute_route(token, root_routes, sizeof(root_routes) / sizeof(root_routes[0]), &u8data[2], length - 2);
}

#include "sim900.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"

//AT+CSMINS? sim status 0,1 - OK

//https://electronics.stackexchange.com/questions/231224/posting-to-php-server-using-sim900-gprs-with-at-commands
struct sim300_context_t;

typedef enum simx_cmd_t
{
    NO_AT = -1,
    AT_CPINR = 0x0,
    AT_CREG,
    AT_CMGF,
    AT_AT,
    AT_CMGS,
    AT_CSMINS,
    AT_CGATT,
    AT_CIPSHUT,
    AT_CIPSTATUS,
    AT_CIPMUX,
    AT_CSTT,
    AT_CIICR,
    AT_CIFSR,
    AT_CSQ,
    AT_CIPSTART,
    AT_CIPSEND,
    AT_CIPCLOSE,
    AT_CIPHEAD,
    AT_CSCS,
    AT_CMGR,
    AT_CGDCONT,
    ATD,
    ATO,
    PPP,
    AT_CIPCSGP,
}simx_cmd_t;

typedef struct
{
    char *str;
    uint8_t size;
}str_tocken_t;

char* str_quotes(const char *src, char *dst, uint16_t maxlen);
void simx_finished(struct sim300_context_t *context);
void _simx_tcp_head_enable_finished(struct sim300_context_t *context);
void _simx_multiple_connection_finished(struct sim300_context_t *context);
void _simx_set_TE_character_finished(struct sim300_context_t *context);
void sim300_send_at_cmd_vp(struct sim300_context_t *context, sim_reply_t *reply, simx_cmd_t cmd, const char * format, ... );
void sim300_send_cmd_vp(struct sim300_context_t *context, sim_reply_t *reply, simx_cmd_t cmd, const char * format, ... );
void sim300_send_no_at_cmd(struct sim300_context_t *context, sim_reply_t *reply, simx_cmd_t cmd);
void sim300_send_at_cmd(struct sim300_context_t *context, sim_reply_t *reply, simx_cmd_t cmd);
uint8_t simx_is_receive(void);
void _simx_current_connection_status_parse(struct sim300_context_t *context, char *buffer, uint16_t length);
void _strchrcpy(char *src, str_tocken_t *tocken, uint8_t size);
uint8_t str_to_ip(char *strip, sim_ip_t *ip);
sim_tcp_mode_t str_to_tcp_mode(uint8_t *str);

void simx_rcv_rframe(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void simx_rcv_wframe(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void simx_rcv_dframe(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void simx_rcv_mframe(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void simx_rcv_ipframe(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void simx_rcv_sdframe(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);

void _simx_pin_is_required_resp(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void _simx_network_registration_resp(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void _simx_sim_inserted_status_resp(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void _simx_is_attach_to_GPRS_resp(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void _simx_current_connection_status_resp(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void _simx_signal_quality_report_resp(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void _simx_sms_read_resp(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);

void _simx_notification_receive(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void _simx_notification_sms(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);
void _simx_notification_pdp_deact(struct sim300_context_t *context, uint8_t *buffer, uint16_t length);

void simx_receive_tcp(struct sim300_context_t *context, uint8_t byte);
void simx_receive_msg(struct sim300_context_t *context, uint8_t byte);

void (*fsimx_receive)(struct sim300_context_t *context, uint8_t byte) = simx_receive_msg;


uint8_t g_sim_rx_buffer[SIM_RX_BUFFER];
uint8_t g_sim_tx_buffer[SIM_TX_BUFFER];
uint8_t sim_cnt;
uint16_t g_receive_len;
uint8_t g_receive_ncon;

typedef enum
{
    AT_CMD_ST_RN,
    AT_CMD_ST_CMD,
    AT_CMD_ST_MSG,
    AT_CMD_ST_STATUS,
}at_cmd_state_t;

typedef void(*fcmd_frame)(struct sim300_context_t *context, uint8_t *msg, uint16_t length);
typedef void(*fcmd_finished_t)(struct sim300_context_t *context);


fcmd_finished_t fcmd_finished = NULL;

typedef struct
{
    simx_cmd_t cmd;
    char *cmdstr;
    uint8_t len;
    fcmd_frame f_frm;
    fcmd_frame f_msg;
}simx_cmd_spec_t;

typedef struct
{
    char *cmdstr;
    uint8_t len;
    fcmd_frame f;
}simx_notif_spec_t;

typedef struct
{
    char *cmdstr;
    uint8_t len;
    sim_status_t status;
}sim_status_spec_t;

typedef struct sim300_context_t
{
    simx_cmd_t cmd;
    sim_reply_t *reply;
    volatile uint8_t is_receive;
    volatile at_cmd_state_t state;
    char* cmd_data;
    
    uint8_t mux;
    uint8_t ciphead;
    uint8_t cipshowtp;
    sim_TE_chaster_t te_chaster;
    sim_pin_status_t pin_is_required;
    sim_reg_network_t network_reg;
    int8_t sim_is_insert;
    int8_t gprs_is_attach;
    sim_cip_state_t cip_state;
    volatile uint32_t time_ms;
}sim300_context_t;

#define CMD_STR(a) .cmdstr = a, .len = sizeof(a) - 1
simx_cmd_spec_t simx_cmd_spec[] = 
    {{.cmd = AT_CPINR,     CMD_STR("CPIN?"),     .f_frm = simx_rcv_rframe,  .f_msg = _simx_pin_is_required_resp},
     {.cmd = AT_CREG,      CMD_STR("CREG?"),     .f_frm = simx_rcv_rframe,  .f_msg = _simx_network_registration_resp},
     {.cmd = AT_CMGF,      CMD_STR("CMGF="),     .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = AT_AT,        CMD_STR("AT"),        .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = AT_CMGS,      CMD_STR("CMGS="),     .f_frm = simx_rcv_wframe,  .f_msg = NULL},
     {.cmd = AT_CSMINS,    CMD_STR("CSMINS?"),   .f_frm = simx_rcv_rframe,  .f_msg = _simx_sim_inserted_status_resp},
     {.cmd = AT_CGATT,     CMD_STR("CGATT?"),    .f_frm = simx_rcv_rframe,  .f_msg = _simx_is_attach_to_GPRS_resp},
     {.cmd = AT_CIPSHUT,   CMD_STR("CIPSHUT"),   .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = AT_CIPSTATUS, CMD_STR("CIPSTATUS"), .f_frm = simx_rcv_mframe,  .f_msg = _simx_current_connection_status_resp},
     {.cmd = AT_CIPMUX,    CMD_STR("CIPMUX="),   .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = AT_CSTT,      CMD_STR("CSTT="),     .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = AT_CIICR,     CMD_STR("CIICR"),     .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = AT_CIFSR,     CMD_STR("CIFSR"),     .f_frm = simx_rcv_ipframe, .f_msg = NULL},
     {.cmd = AT_CSQ,       CMD_STR("CSQ"),       .f_frm = simx_rcv_rframe , .f_msg = _simx_signal_quality_report_resp},
     {.cmd = AT_CIPSTART,  CMD_STR("CIPSTART="), .f_frm = simx_rcv_dframe , .f_msg = NULL},
     {.cmd = AT_CIPSEND,   CMD_STR("CIPSEND="),  .f_frm = simx_rcv_sdframe, .f_msg = NULL},
     {.cmd = AT_CIPCLOSE,  CMD_STR("CIPCLOSE="), .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = AT_CIPHEAD,   CMD_STR("CIPHEAD="),  .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = AT_CSCS,      CMD_STR("CSCS="),     .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = AT_CMGR,      CMD_STR("CMGR="),     .f_frm = simx_rcv_rframe,  .f_msg = _simx_sms_read_resp},
     {.cmd = AT_CGDCONT,   CMD_STR("CGDCONT="),  .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = ATD,          CMD_STR("ATD"),       .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = ATO,          CMD_STR("ATO"),       .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = PPP,          CMD_STR("PPP"),       .f_frm = simx_rcv_dframe,  .f_msg = NULL},
     {.cmd = AT_CIPCSGP,   CMD_STR("CIPCSGP="),  .f_frm = simx_rcv_dframe,  .f_msg = NULL}};



simx_notif_spec_t simx_notif_spec[] = 
    {{CMD_STR("+RECEIVE,"),   .f = _simx_notification_receive},
     {CMD_STR("+CMTI:"),      .f = _simx_notification_sms},
     {CMD_STR("+PDP: DEACT"), .f = _simx_notification_pdp_deact}};

sim_status_spec_t sim_status_spec[] = 
    {{CMD_STR("OK\r\n"), .status = SIM300_OK},
    {CMD_STR("ERROR\r\n"), .status = SIM300_ERROR},
    {CMD_STR("SHUT OK\r\n"), .status = SIM300_SHUT_OK},
    {CMD_STR("SEND OK\r\n"), .status = SIM300_SEND_OK},
    {CMD_STR("SEND FAIL\r\n"), .status = SIM300_SEND_FAIL},
    {CMD_STR("CLOSE OK\r\n"), .status = SIM300_CLOSE_OK},
    {CMD_STR("NO DIALTONE\r\n"), .status = SIM300_NO_DIALTONE},
    {CMD_STR("BUSY\r\n"), .status = SIM300_BUSY},
    {CMD_STR("NO CARRIER\r\n"), .status = SIM300_NO_CARRIER},
    {CMD_STR("NO ANSWER\r\n"), .status = SIM300_NO_ANSWER},
    {CMD_STR("CONNECT\r\n"), .status = SIM300_CONNECT},};

static const char* te_chasters[] = {"GSM", "UCS2", "IRA", "HEX", "PCCP", "PCDN", "8859-1"};

sim300_context_t g_context = {.pin_is_required = SIM_PIN_UNKNOW, .is_receive = 1, .cmd = NO_AT, .mux = 0};

void simx_receive_frame(sim300_context_t *context, uint8_t *buffer, uint16_t length);

/***********************UTILS********************************/
char* str_quotes(const char *src, char *dst, uint16_t maxlen)
{
    if(src == NULL)
    {
        return NULL;
    }
    char *c1 = strchr(src, '"');
    if(c1 == NULL)
    {
        return NULL;
    }
    c1++;
    char *c2 = strchr(c1, '"');
    if(c2 == NULL)
    {
        return NULL;
    }
    unsigned int len = (unsigned int)c2 - (unsigned int)c1;
    if(len == 1)
    {
        dst[0] = 0;
        return c2;
    }
    if(len < maxlen)
    {
        memcpy(dst, c1, len);
        dst[len] = 0;
        return c1 + len + 1;
    }
    else
    {
        return c2;
    }
}

void _strchrcpy(char *src, str_tocken_t *tocken, uint8_t size)
{
    uint32_t n = strlen(src);
    uint8_t cnt = 0;
    if(src[0] != ',')
    {
        tocken[cnt].str = src;
    }
    
    for(int i = 0; i < n; i++)
    {
        if(src[i] == ',')
        {
            if(++cnt == size)
            {
                break;
            }
            tocken[cnt].str = src + i + 1;
        }
        else
        {
            tocken[cnt].size++;
        }
    }
}



uint8_t str_to_ip(char *strip, sim_ip_t *ip)
{
    int v1, v2, v3, v4;
    int n = sscanf(strip, "%i.%i.%i.%i", &v1, &v2, &v3, &v4);
    if(n == 4)
    {
        ip->addr0 = v1;
        ip->addr1 = v2;
        ip->addr2 = v3;
        ip->addr3 = v4;
        return 0;
    }
    return -1;
}

sim_tcp_mode_t str_to_tcp_mode(uint8_t *str)
{
    sim_tcp_mode_t mode;
    if(memcmp(str, "TCP", 3) == 0)
    {
        mode = SIM_TCP;
    }
    else if(memcmp(str, "UDP", 3) == 0)
    {
        mode = SIM_UDP;
    }
    else
    {
        mode = -1;
    }
    return mode;
}
/*******************************************************/

void simx_tick_1ms(void)
{
    if(g_context.is_receive == 0)
    {
        if(++g_context.time_ms > SIM900_TIMEOUT)
        {
            g_context.reply->status = SIM300_TIMEOUT;
            g_context.is_receive == 1;
            simx_callback_timeout();
        }
    }
}

void simx_finished(sim300_context_t *context)
{
     context->is_receive = 1;
     if(fcmd_finished != NULL)
     {
         fcmd_finished(context);
         fcmd_finished = NULL;
     }
}

void simx_rcv_rframe(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    //printf("frame: %s\n", buffer);
    //printf("frame: %s state: %i\n", buffer, context->state);

    switch(context->state)
    {
    case AT_CMD_ST_RN:
        if(length == 2)
        {
            context->state = AT_CMD_ST_CMD;
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
            simx_finished(context);
        }
        break;
    case AT_CMD_ST_CMD:
        
        if(memcmp(buffer + 1, simx_cmd_spec[context->cmd].cmdstr, 
                  simx_cmd_spec[context->cmd].len - 1) == 0)
        {
            //printf("frame: %s\n", simx_cmd_spec[context->cmd].cmdstr);
            context->state = AT_CMD_ST_MSG;
            if(simx_cmd_spec[context->cmd].f_msg != NULL)
            {
                simx_cmd_spec[context->cmd].f_msg(context, buffer, length);
            }
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
            simx_finished(context);
        }
        break;
    case AT_CMD_ST_MSG:
        //printf("msg: %s\n", buffer);
        if(length == 2)
        {
            
            context->state = AT_CMD_ST_STATUS;
        }
        else
        {
            if(simx_cmd_spec[context->cmd].f_msg != NULL)
            {
                simx_cmd_spec[context->cmd].f_msg(context, buffer, length);
            }
        }
        break;
    case AT_CMD_ST_STATUS:
        //printf("status: %s\n", buffer);
        if(memcmp(buffer, "OK\r\n", 4) == 0)
        {
            context->reply->status = SIM300_OK;
        }
        else if(memcmp(buffer, "ERROR\r\n", 7) == 0)
        {
            context->reply->status = SIM300_ERROR;
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
        }
        simx_finished(context);
        break;
    }
}

void simx_rcv_dframe(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    switch(context->state)
    {
    case AT_CMD_ST_RN:
        if(length == 2)
        {
            context->state = AT_CMD_ST_STATUS;
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
            simx_finished(context);
        }
        break;
    case AT_CMD_ST_STATUS:
        
        for(uint8_t i = 0; i < sizeof(sim_status_spec) / sizeof(sim_status_spec[0]); i++)
        {
            if(memcmp(buffer, sim_status_spec[i].cmdstr, sim_status_spec[i].len) == 0)
            {
                context->reply->status = sim_status_spec[i].status;
                simx_finished(context);
                return;
            }
        }
        
        uint8_t *tmp = buffer;
        uint16_t len = length;
        if(context->mux)
        {
            tmp += 3;
            len -= 3;
        }
        if(memcmp(tmp, "CLOSE OK\r\n", len) == 0)
        {
            context->reply->status = SIM300_CLOSE_OK;
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
        }
        
        simx_finished(context);
        break;
    }
}

void simx_rcv_wframe(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    switch(context->state)
    {
    case AT_CMD_ST_RN:
        if(length == 2)
        {
            context->state = AT_CMD_ST_MSG;
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
            simx_finished(context);
        }
        break;
    case AT_CMD_ST_CMD:
        
        if(length == 2 && buffer[0] == '\r' && buffer[1] == '\n')
        {
            context->state = AT_CMD_ST_STATUS;
        } 
        else if(memcmp(buffer + 1, simx_cmd_spec[context->cmd].cmdstr, 
                  simx_cmd_spec[context->cmd].len - 1) == 0)
        {
            if(simx_cmd_spec[context->cmd].f_msg != NULL)
            {
                simx_cmd_spec[context->cmd].f_msg(context, buffer, length);
            }
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
            simx_finished(context);
        }
        break;
    case AT_CMD_ST_MSG:
        if(length == 2 && buffer[0] == '\r' && buffer[1] == '\n')
        {
            context->state = AT_CMD_ST_CMD;
        } 
        else if(buffer[0] == '>')
        {
            sprintf((char*)g_sim_tx_buffer, "%s%c", g_context.cmd_data, 0x1A); //TODO KOSTIL
            simx_callback_send(g_sim_tx_buffer, strlen((const char*)g_sim_tx_buffer));
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
            simx_finished(context);
        }
        break;
    case AT_CMD_ST_STATUS:
        //printf("status: %s\n", buffer);
        if(memcmp(buffer, "OK\r\n", 4) == 0)
        {
            context->reply->status = SIM300_OK;
        }
        else if(memcmp(buffer, "ERROR\r\n", 7) == 0)
        {
            context->reply->status = SIM300_ERROR;
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
        }
        simx_finished(context);
        break;
    }
}

void simx_rcv_mframe(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    switch(context->state)
    {
    case AT_CMD_ST_RN:
        if(length == 2)
        {
            context->state = AT_CMD_ST_STATUS;
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
            simx_finished(context);
        }
        break;
    case AT_CMD_ST_MSG:
        
        if(simx_cmd_spec[context->cmd].f_msg != NULL)
        {
            simx_cmd_spec[context->cmd].f_msg(context, buffer, length);
        }
        
        break;
    case AT_CMD_ST_STATUS:
        if(memcmp(buffer, "OK\r\n", 4) == 0)
        {
            context->reply->status = SIM300_OK;
            context->state = AT_CMD_ST_MSG;
        }
        else if(memcmp(buffer, "ERROR\r\n", 7) == 0)
        {
            context->reply->status = SIM300_ERROR;
            simx_finished(context);
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
            simx_finished(context);
        }
        break;
    }
}

void simx_rcv_ipframe(struct sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    int addr0, addr1, addr2, addr3, n;
    switch(context->state)
    {
    case AT_CMD_ST_RN:
        if(length == 2)
        {
            context->state = AT_CMD_ST_MSG;
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
            simx_finished(context);
        }
        break;
    case AT_CMD_ST_MSG:
        
        n = sscanf((char const*)buffer, "%i.%i.%i.%i", &addr0, &addr1, &addr2, &addr3);
        if(n == 4)
        {
            sim_ip_t *ip = (sim_ip_t *)context->reply->user_pointer;
            ip->addr0 = addr0;
            ip->addr1 = addr1;
            ip->addr2 = addr2;
            ip->addr3 = addr3;
            context->reply->status = SIM300_OK;
        }
        else
        {
            context->reply->status = SIM300_ERRFRAME;
        }
        simx_finished(context);
        break;
    }
}

void simx_rcv_sdframe(struct sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    switch(context->state)
    {
    case AT_CMD_ST_RN:
        context->state = AT_CMD_ST_MSG;
        break;
    case AT_CMD_ST_MSG:
        if(length == 2)
        {
            if(buffer[0] == '>')
            {
                for(uint32_t i = 0; i < 500000; i++)
                {
                    asm("nop");
                }
                simx_callback_send((uint8_t*)context->reply->user_pointer, context->reply->user_data);
            }
        }
        else 
        {
            uint8_t *tmp = buffer;
            uint16_t len = length;
            if(context->mux)
            {
                tmp += 3;
                len -= 3;
            }
            if(memcmp(tmp, "SEND OK\r\n", len) == 0)
            {
                context->reply->status = SIM300_SEND_OK;
                simx_finished(context);
            }
            else if(memcmp(tmp, "SEND FAIL\r\n", len) == 0)
            {
                context->reply->status = SIM300_SEND_FAIL;
                simx_finished(context);
            }
        }
       
        break;
    }
}

void _simx_notification_receive(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    if(context->mux && length > 11)
    {
        int size, desc;
        uint8_t n = sscanf((char const*)buffer + 9, "%i,%i", &desc, &size);
        if(n == 2)
        {
            g_receive_ncon = desc;
            g_receive_len = size;
            fsimx_receive = simx_receive_tcp;
        }
    }
}

void _simx_notification_sms(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    //TODO ADD MEM TYPE
    if(length > 10)
    {
        char mem[10] = {0};
        int n;
        uint8_t k = sscanf((char const *)(buffer + 6), "%*[^\"]\"%[^\"]\",%d", mem, &n);
        if(k == 2)
        {
            simx_callback_sms_received(n);
        }
        else
        {
            //TODO error frame
        }
    }
}

void _simx_notification_pdp_deact(struct sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    simx_callback_pdp_deact();
}

void simx_receive_frame(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    if(context->is_receive == 1)
    {
        if(length > 2)
        {
            uint8_t n = 0;
            uint16_t len = length;
            char *tmpbuff = (char*)buffer;
            
            for(int i = 0; i < sizeof(simx_notif_spec) / sizeof(simx_notif_spec[0]); i++)
            {
                if(memcmp(buffer, simx_notif_spec[i].cmdstr, simx_notif_spec[i].len) == 0)
                {
                    if(simx_notif_spec[i].f)
                    {
                        simx_notif_spec[i].f(context, buffer, length);
                    }
                    return;
                }
            }
            
            if(context->mux)
            {
                tmpbuff = (char*)(buffer + 3);
                n = buffer[0] - 0x30;
                len -= 3;
            }
            
            if(memcmp(tmpbuff, "CONNECT OK\r\n", len) == 0)
            {
                simx_callback_tcp_msg(SIM_TCP_CONNECT_OK, n);
            }
            else if(memcmp(tmpbuff, "CLOSED\r\n", len) == 0 || 
                    memcmp(tmpbuff, "CLOSE OK\r\n", len) == 0)
            {
                simx_callback_tcp_msg(SIM_TCP_CLOSED, n);
            }
            else if(memcmp(tmpbuff, "ALREADY CONNECT\r\n", len) == 0)
            {
                simx_callback_tcp_msg(SIM_TCP_ALREADY_CONNECT, n);
                
            }
            else if(memcmp(tmpbuff, "CONNECT FAIL\r\n", len) == 0)
            {
                simx_callback_tcp_msg(SIM_TCP_CONNECT_FAIL, n);
            }
        }
        return;
    }
    
    if(sizeof(simx_cmd_spec) / sizeof(simx_cmd_spec[0]) > context->cmd && context->cmd >= 0)
    {
        if(simx_cmd_spec[context->cmd].f_frm != NULL)
        {
            simx_cmd_spec[context->cmd].f_frm(context, buffer, length);
        }
    }
    else
    {
        //TODO add custom responce
    }
}

void simx_receive_tcp(sim300_context_t *context, uint8_t byte)
{
    g_sim_rx_buffer[sim_cnt] = byte;
    if(++sim_cnt == g_receive_len) //todo overflow
    {
        g_sim_rx_buffer[sim_cnt] = 0;
        simx_callback_tcp_data(g_sim_rx_buffer, g_receive_len, g_receive_ncon);
        g_receive_len = 0;
        g_receive_ncon = 0;
        fsimx_receive = simx_receive_msg;
        sim_cnt = 0;
        //callback
    }
}

void simx_receive_msg(sim300_context_t *context, uint8_t byte)
{
    g_sim_rx_buffer[sim_cnt] = byte;
    sim_cnt++;
    
    
    if(simx_cmd_spec[context->cmd].f_frm == simx_rcv_wframe || 
       simx_cmd_spec[context->cmd].f_frm == simx_rcv_sdframe)
    {
        if(g_sim_rx_buffer[0] == '>' && sim_cnt == 2)
        {
            g_sim_rx_buffer[sim_cnt] = 0;
            simx_receive_frame(context, g_sim_rx_buffer, sim_cnt);
            sim_cnt = 0;
        }
    }
    
    if(g_sim_rx_buffer[sim_cnt - 2] == '\r' && 
       g_sim_rx_buffer[sim_cnt - 1] == '\n' )
    {
        g_sim_rx_buffer[sim_cnt] = 0;
        //printf("rec: [%s]\n", g_sim_rx_buffer);
        simx_receive_frame(context, g_sim_rx_buffer, sim_cnt);
        sim_cnt = 0;
    }
    else if(context->mux == 0 && context->ciphead && 
            context->is_receive == 1 && g_sim_rx_buffer[sim_cnt - 1] == ':')
    {
        if(memcmp(g_sim_rx_buffer, "+IPD,", 5) == 0)
        {
            int size;
            sscanf((char*)(g_sim_rx_buffer + 5), "%i", &size);
            g_receive_ncon = 0;
            g_receive_len = size;
            sim_cnt = 0;
            fsimx_receive = simx_receive_tcp;
        }
    }
}

void simx_receive(uint8_t byte)
{
    fsimx_receive(&g_context, byte);
}



uint8_t simx_is_receive()
{
    return g_context.is_receive;
}

void simx_wait_reply()
{
    while(g_context.is_receive == 0)
    {
        if(g_context.time_ms > SIM900_TIMEOUT)
        {
            g_context.is_receive == 1;
            g_context.reply->status = SIM300_TIMEOUT;
            break;
        }
        simx_callback_update();
    }
    for(uint32_t i = 0; i < 500000; i++)
    {
        asm("nop");
    }
}

void sim300_send_no_at_cmd(sim300_context_t *context, sim_reply_t *reply, simx_cmd_t cmd)
{
    context->time_ms = 0;
    fsimx_receive = simx_receive_msg;
    sim_cnt = 0;
    context->is_receive = 0;
    context->state = AT_CMD_ST_RN;
    context->reply = reply;
    context->cmd = cmd;
    reply->status = SIM300_NULL;
    sprintf((char*)g_sim_tx_buffer, "%s\r\n", simx_cmd_spec[cmd].cmdstr);
    simx_callback_send(g_sim_tx_buffer, simx_cmd_spec[cmd].len + 2);
}

void sim300_send_at_cmd(sim300_context_t *context, sim_reply_t *reply, simx_cmd_t cmd)
{
    context->time_ms = 0;
    fsimx_receive = simx_receive_msg;
    sim_cnt = 0;
    context->is_receive = 0;
    context->state = AT_CMD_ST_RN;
    context->reply = reply;
    context->cmd = cmd;
    reply->status = SIM300_NULL;
    sprintf((char*)g_sim_tx_buffer, "AT+%s\r\n", simx_cmd_spec[cmd].cmdstr);
    simx_callback_send(g_sim_tx_buffer, simx_cmd_spec[cmd].len + 5);
}

void sim300_send_cmd_vp(sim300_context_t *context, sim_reply_t *reply, simx_cmd_t cmd, const char * format, ... )
{
    context->time_ms = 0;
    fsimx_receive = simx_receive_msg;
    sim_cnt = 0;
    context->is_receive = 0;
    context->state = AT_CMD_ST_RN;
    context->reply = reply;
    context->cmd = cmd;
    reply->status = SIM300_NULL;
    sprintf((char*)g_sim_tx_buffer, "%s", simx_cmd_spec[cmd].cmdstr);
    uint16_t len = simx_cmd_spec[cmd].len;
    va_list args;
    va_start(args, format);
    vsprintf((char*)g_sim_tx_buffer + len, format, args);
    va_end(args);
    len = strlen((const char*)g_sim_tx_buffer);
    g_sim_tx_buffer[len++] = '\r';
    g_sim_tx_buffer[len++] = '\n';
    g_sim_tx_buffer[len] = 0;
    
    simx_callback_send(g_sim_tx_buffer, len);
}

void sim300_send_at_cmd_vp(sim300_context_t *context, sim_reply_t *reply, simx_cmd_t cmd, const char * format, ... )
{
    context->time_ms = 0;
    fsimx_receive = simx_receive_msg;
    sim_cnt = 0;
    context->is_receive = 0;
    context->state = AT_CMD_ST_RN;
    context->reply = reply;
    context->cmd = cmd;
    reply->status = SIM300_NULL;
    sprintf((char*)g_sim_tx_buffer, "AT+%s", simx_cmd_spec[cmd].cmdstr);
    uint16_t len = simx_cmd_spec[cmd].len + 3;
    va_list args;
    va_start(args, format);
    vsprintf((char*)(g_sim_tx_buffer + len), format, args);
    va_end(args);
    len = strlen((char*)g_sim_tx_buffer);
    g_sim_tx_buffer[len++] = '\r';
    g_sim_tx_buffer[len++] = '\n';
    g_sim_tx_buffer[len] = 0;
    
    simx_callback_send(g_sim_tx_buffer, len);
}

void simx_test(sim_reply_t *reply)
{
    sim300_send_no_at_cmd(&g_context, reply, AT_AT);
}

void _simx_pin_is_required_resp(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    static char *msg[] = {"READY", "SIM PIN", "SIM PUK", "PH_SIM PIN", 
                          "PH_SIM PUK", "SIM PIN2", "SIM PUK2"};
    for(int i = 0; i < sizeof(msg) / sizeof(msg[0]); i++)
    {
        int n = strlen(msg[i]);
        //+CPIN: XXXXXX\r\n\0 == XXXXXX\0
        //printf("length %i %i, %s\n", length - 7, n + 2, msg[i]);
        if(length - 7 == n + 2)
        {
            if(memcmp(buffer + 7, msg[i], n) == 0)
            {
                context->pin_is_required = (sim_pin_status_t)i;// TODO cast
                return;
            }
        }
    }
    context->pin_is_required = SIM_PIN_UNKNOW;
}

void simx_pin_is_required(sim_reply_t *reply)
{
    sim300_send_at_cmd(&g_context, reply, AT_CPINR);
}

void _simx_network_registration_resp(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    int n, stat;
    if(length >= 12)
    {
        sscanf((char*)(buffer + 7), "%i,%i", &n, &stat);
        if(stat >= 0 && stat <= 5)
        {
            context->network_reg = (sim_reg_network_t)stat;
        }
        else
        {
            context->network_reg = (sim_reg_network_t)-1;
        }
    }
    else
    {
        context->network_reg = (sim_reg_network_t)-1;
    }
}

void simx_network_registration(sim_reply_t *reply)
{
    sim300_send_at_cmd(&g_context, reply, AT_CREG);
}

void _simx_sim_inserted_status_resp(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    //printf("rec: %s %i", buffer, length);
    int n, stat;
    if(length >= 13)
    {
        
        sscanf((char*)(buffer + 9), "%i,%i", &n, &stat);
        if(stat >= 0 && stat <= 1)
        {
            context->sim_is_insert = stat;
        }
        else
        {
            context->sim_is_insert = -1;
        }
    }
    else
    {
        context->sim_is_insert = -1;
    }
}

void simx_sim_inserted_status(sim_reply_t *reply)
{
    sim300_send_at_cmd(&g_context, reply, AT_CSMINS);
}

void _simx_signal_quality_report_resp(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    int q, n, l;
    if(length >= 10)
    {
        
        n = sscanf((char*)(buffer + 6), "%i,%i", &q, &l);
        if(n == 2 && context->reply->user_pointer != NULL)
        {
            *((uint8_t*)context->reply->user_pointer) = q;
        }
    }
}

void simx_signal_quality_report(sim_reply_t *reply, uint8_t *lvl)
{
    reply->user_pointer = lvl;
    sim300_send_at_cmd(&g_context, reply, AT_CSQ);
}

void _simx_set_TE_character_finished(sim300_context_t *context)
{
    context->te_chaster = (sim_TE_chaster_t)context->reply->user_data;
}

void simx_set_TE_character(sim_reply_t *reply, sim_TE_chaster_t chaster)
{
    fcmd_finished = _simx_set_TE_character_finished;
    reply->user_data = chaster;
    sim300_send_at_cmd_vp(&g_context, reply, AT_CSCS, "\"%s\"", te_chasters[chaster]);
}

void simx_call_to_dial_number(sim_reply_t *reply, char *number)
{
    sim300_send_cmd_vp(&g_context, reply, ATD, "%s", number);
}

void simx_switch_mode(sim_reply_t *reply, sim_pdp_mode_t mode)
{
    if(mode == SIM_DATA_MODE)
    {
        sim300_send_no_at_cmd(&g_context, reply, ATO);
    }
    else
    {
        fsimx_receive = simx_receive_msg;
        sim_cnt = 0;
        g_context.is_receive = 0;
        g_context.state = AT_CMD_ST_RN;
        g_context.reply = reply;
        g_context.cmd = PPP;
        reply->status = SIM300_NULL;
        sprintf((char*)g_sim_tx_buffer, "+++");
        simx_callback_send(g_sim_tx_buffer, 3);
    }
}

void _simx_is_attach_to_GPRS_resp(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    int n;
    if(length >= 8)
    {
        
        sscanf((char*)(buffer + 8), "%i", &n);
        if(n >= 0 && n <= 1)
        {
            context->gprs_is_attach = n;
        }
        else
        {
            context->gprs_is_attach = -1;
        }
    }
    else
    {
        context->gprs_is_attach = -1;
    }
}

void simx_is_attach_to_GPRS(sim_reply_t *reply)
{
    sim300_send_at_cmd(&g_context, reply, AT_CGATT);
}


void simx_deactivate_gprs_pdp(sim_reply_t *reply)
{
    sim300_send_at_cmd(&g_context, reply, AT_CIPSHUT);
}

void simx_set_gprs_config(sim_reply_t *reply, const char* apn, const char* username, const char* pass)
{
    sim300_send_at_cmd_vp(&g_context, reply, AT_CSTT, "\"%s\",\"%s\",\"%s\"", apn, username, pass);
}

void simx_bring_up_wireless_connection(sim_reply_t *reply)
{
    sim300_send_at_cmd(&g_context, reply, AT_CIICR);
}

void simx_get_local_ip(sim_reply_t *reply, sim_ip_t *ip)
{
    reply->user_pointer = ip;
    sim300_send_at_cmd(&g_context, reply, AT_CIFSR);
}

void simx_define_pdp_context(sim_reply_t *reply, uint8_t cid, char *pdp_type, char *apn)
{
    sim300_send_at_cmd_vp(&g_context, reply, AT_CGDCONT, "%i,\"%s\",\"%s\"", cid, pdp_type, apn);
}

void simx_set_connection_mode(sim_reply_t *reply, sim_connection_mode_t mode, char *apn, 
                              char *user_name, char *pass, sim_csd_rate_t rate)
{
    if(mode == SIM_CSD)
    {
        sim300_send_at_cmd_vp(&g_context, reply, AT_CIPCSGP, 
                              "%i,\"%s\",\"%s\",\"%s\",%i", mode, apn, user_name, pass, rate);
    }
    else if(mode == SIM_GPRS)
    {
        sim300_send_at_cmd_vp(&g_context, reply, AT_CIPCSGP, 
                              "%i,\"%s\",\"%s\",\"%s\"", mode, apn, user_name, pass);
    }
    else
    {
        //TODO ERROR
    }
}

void _simx_multiple_connection_finished(sim300_context_t *context)
{
    if(context->reply->status == SIM300_OK)
    {
        context->mux = context->reply->user_data;
    }
}
/*
void _simx_current_connection_status_parse(sim300_context_t *context, char *buffer, uint16_t length)
{
    if(length < 17)
    {
        return;
    }
    char mem[4][19] = {0};
    int n1, n2 = 0;
    char c;
    uint8_t cmtp = 0;
    c = buffer[0];
    n1 = buffer[3] - 0x30;
    printf("_simx_current_connection_status_parse: %s", buffer);
    for(uint8_t i = 0; i < 10; i++)
    {
        if(buffer[i] == ',')
        {
            if(++cmtp == 2)
            {
                buffer += i + 1;
                break;
            }
        }
    }
    
    int k = sscanf(buffer, "\"%[^\"]\",\"%[^\"]\",\"%[^\"]\",\"%[^\"]\"", 
                   mem[0], mem[1], mem[2], mem[3]);
    if(k == 4)
    {
        sim_cipstatus_t *st = context->reply->user_pointer;
        uint8_t stsize = context->reply->user_data;
        if(stsize > n1)
        {
            st[n1].type = c;
            st[n1].n = n1;
            st[n1].bearer = n2;
            st[n1].mode = str_to_tcp_mode(mem[0]);
            str_to_ip(mem[1], &st[n1].ip);
            int port;
            sscanf(mem[2], "%i", &port);
            st[n1].port = port;
        }
    }
    if(n1 == 7)
    {
        g_context.is_receive = 1;
    }
}*/

void _simx_current_connection_status_parse(sim300_context_t *context, char *buffer, uint16_t length)
{
    if(length < 17)
    {
        return;
    }
    
    sim_cipstatus_t *st = context->reply->user_pointer;
    uint8_t stsize = context->reply->user_data;
    
    char c1 = buffer[0];
    int n1 = buffer[3] - 0x30;
    if(stsize > n1)
    {
    
        st[n1].n = n1;
        st[n1].type = c1;
        
        str_tocken_t mem[8] = {0};
        char *c = buffer + 5;
        
        _strchrcpy(c, mem, 20);
        if(mem[0].size != 0)
        {
            st[n1].bearer = mem[0].str[0] - 0x30;
        }
        if(mem[1].size > 2)
        {
            st[n1].mode = str_to_tcp_mode((uint8_t*)mem[1].str + 1);
        }
        if(mem[2].size > 2)
        {
            str_to_ip(mem[2].str + 1, &st[n1].ip);
        }
        if(mem[3].size > 2)
        {
            int port;
            sscanf(mem[3].str + 1, "%i", &port);
            st[n1].port = port;
        }
        
        /*static const sim_mux_cip_state_t cip_state[] = {
            MUX_CIP_IP_INITIAL, MUX_CIP_IP_START, MUX_CIP_IP_CONFIG, 
            MUX_CIP_IP_GPRSACT, MUX_CIP_IP_STATUS, MUX_CIP_IP_PROCESSUNG, MUX_CIP_PDP_DEACT};
        
        static const char *cipstate[] = {"IP INITIAL",
            "IP START", "IP CONFIG", "IP GPRSACT", "IP STATUS",
            "IP PROCESSUNG", "PDP DEACT"};*/
        static const sim_cip_client_state_t cip_state[] = {
                CCP_INITIAL, CCP_CONNECTING, CCP_CONNECTED, 
                CCP_REMOTE_CLOSING, CCP_CLOSING, CCP_CLOSED};
            
        static const char *cipstate[] = {
            "INITIAL", "CONNECTING", "CONNECTED", "REMOTE CLOSING",
            "CLOSING", "CLOSED"};
          
        if(mem[4].size > 2)
        {
            for(uint8_t i = 0; i < sizeof(cip_state) / sizeof(cip_state[0]); i++)
            {
                if(memcmp(mem[4].str + 1, cipstate[i], strlen(cipstate[i])) == 0)
                {
                    st[n1].state = cip_state[i];
                    break;
                }
            }
        }
    }
    
    if(n1 == 7)
    {
        g_context.is_receive = 1;
    }
}

void _simx_current_connection_status_resp(sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    if(context->cmd == AT_CIPSTATUS)
    {
        if(memcmp(buffer, "STATE: ", 7) == 0)
        {
            static const sim_cip_state_t cip_state[] = {CIP_IP_INITIAL,
                CIP_IP_START, CIP_IP_CONFIG, CIP_IP_GPRSACT, CIP_IP_STATUS,
                CIP_CONN_LISTENING, CIP_CONN_LISTENING, CIP_CONN_LISTENING,
                CIP_CONNECT_OK, CIP_CLOSING, CIP_CLOSING, CIP_CLOSED, 
                CIP_CLOSED, CIP_PDP_DEACT};
            static const char *cipstate[] = {"IP INITIAL\0",
                "IP START\0", "IP CONFIG\0", "IP GPRSACT\0", "IP STATUS\0",
                "TCP CONNECTING\0", "UDP CONNECTING\0", "SERVER LISTENING\0", 
                "CONNECT OK\0", "TCP CLOSING\0", "UDP CLOSING\0", "TCP CLOSED\0",
                "UDP CLOSED\0", "PDP DEACT\0"};
            
            for(uint8_t i = 0; i < sizeof(cip_state) / sizeof(cip_state[0]); i++)
            {
                if(memcmp(buffer + 7, cipstate[i], strlen(cipstate[i])) == 0)
                {
                    context->cip_state = cip_state[i];
                    if(g_context.mux == 0)
                    {
                        g_context.is_receive = 1;
                    }
                    return;
                }
            }
            context->cip_state = (sim_cip_state_t) - 1;
            if(g_context.mux == 0)
            {
                g_context.is_receive = 1;
            }
        }
        else
        {
            _simx_current_connection_status_parse(context, (char*)buffer, length);
        }
    }
}

void simx_current_connection_status(sim_reply_t *reply, sim_cipstatus_t *cipstatus, uint8_t size)
{
    reply->user_data = size;
    reply->user_pointer = cipstatus;
    sim300_send_at_cmd(&g_context, reply, AT_CIPSTATUS);
}

void simx_multiple_connection(sim_reply_t *reply, uint8_t mode)
{
    //TODO assert(); mode
    fcmd_finished = _simx_multiple_connection_finished;
    reply->user_data = mode;
    sim300_send_at_cmd_vp(&g_context, reply, AT_CIPMUX, "%i", mode);
}

void simx_tcp_connect(sim_reply_t *reply, sim_tcp_mode_t tcp_mode, char *address, uint16_t port, int n)
{
    char *mode[] = {"TCP", "UDP"};
    uint8_t index = tcp_mode == SIM_TCP ? 0 : 1;
    //TODO assert n = 0..7
    
    if(g_context.mux)
    {
        sim300_send_at_cmd_vp(&g_context, reply, AT_CIPSTART, "%i,\"%s\",\"%s\",%i", 
                            n, mode[index], address, port);
    }
    else
    {
        sim300_send_at_cmd_vp(&g_context, reply, AT_CIPSTART, "\"%s\",\"%s\",%i", 
                            mode[index], address, port);
    }
}

void simx_tcp_send_data(sim_reply_t *reply, uint8_t *data, uint16_t length, int n)
{
    reply->user_pointer = data;
    reply->user_data = length;
    if(g_context.mux)
    {
        sim300_send_at_cmd_vp(&g_context, reply, AT_CIPSEND, "%i,%i", n, length);
    }
    else
    {
        sim300_send_at_cmd_vp(&g_context, reply, AT_CIPSEND, "%i", length);
    }
}

void simx_tcp_close(sim_reply_t *reply, int n)
{
    g_context.mux
        ? sim300_send_at_cmd_vp(&g_context, reply, AT_CIPCLOSE, "%i,%i", n, 0)
        : sim300_send_at_cmd_vp(&g_context, reply, AT_CIPCLOSE, "%i", 0);
}

void _simx_tcp_head_enable_finished(sim300_context_t *context)
{
    if(context->reply->status == SIM300_OK)
    {
        context->ciphead = context->reply->user_data;
    }
}

void simx_tcp_head_enable(sim_reply_t *reply, uint8_t is_enable)
{
    is_enable = is_enable ? 1 : 0;
    g_context.reply = reply;
    g_context.reply->user_data = is_enable;
    fcmd_finished = _simx_tcp_head_enable_finished;
    sim300_send_at_cmd_vp(&g_context, reply, AT_CIPHEAD, "%i", is_enable);
}

void simx_sms_mode(sim_reply_t *reply, sim_sms_mode_t mode)
{
    //TODO assert mode
    sim300_send_at_cmd_vp(&g_context, reply, AT_CMGF, "%i", mode);
}

void simx_send_sms(sim_reply_t *reply, const char *number, const char* msg)
{
    //todo validate number
    g_context.cmd_data = (char*)msg;
    sim300_send_at_cmd_vp(&g_context, reply, AT_CMGS, "\"%s\"", number);
}

void _simx_sms_read_resp(struct sim300_context_t *context, uint8_t *buffer, uint16_t length)
{
    sim_sms_t *sms = (sim_sms_t*)context->reply->user_pointer;
    if(memcmp(buffer, "+CMGR:", sizeof("+CMGR:") - 1) == 0)
    {
        char strstaus[10];
        char *c = (char*)buffer;
        c = str_quotes(c, strstaus, 10);
        if(c == NULL) return;
        c = str_quotes(c + 1, sms->number, 21);
        if(c == NULL) return;
        c = str_quotes(c + 1, sms->mt, 17);
        if(c == NULL) return;
        c = str_quotes(c + 1, sms->date, 22);
    }
    else
    {
        uint16_t len = sms->msg_length > length ? length - 2 : sms->msg_length;
        memcpy(sms->msg, buffer, len);
        sms->msg[len] = 0;
    }
}

void simx_read_sms(sim_reply_t *reply, sim_sms_t *sim_sms, uint16_t n)
{
    reply->user_pointer = sim_sms;
    sim300_send_at_cmd_vp(&g_context, reply, AT_CMGR, "%i", n);
}

sim_pin_status_t sim_pin_required()
{
    return g_context.pin_is_required;
}

sim_reg_network_t sim_network_registration()
{
    return g_context.network_reg;
}

int8_t sim_is_inserted()
{
    return g_context.sim_is_insert;
}

int8_t gprs_is_attach()
{
    return g_context.gprs_is_attach;
}

sim_cip_state_t current_connection_status()
{
    return g_context.cip_state;
}

uint8_t sim_cip_mux_mode()
{
    return g_context.mux;
}

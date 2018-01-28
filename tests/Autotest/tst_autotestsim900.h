#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "sim900.h"


using namespace testing;
char *g_data;
uint16_t g_length;
sim_con_status_t g_con_status;
uint8_t g_con_n = -1;
char* g_tcp_data;
uint16_t g_tcp_length;
uint8_t g_tcp_con;
uint16_t g_sms_number;
uint8_t is_deact;

void simx_callback_sms_received(uint16_t number)
{
    g_sms_number = number;
}

void simx_callback_send(uint8_t *data, uint16_t length)
{
    g_data = (char*)data;
    g_length = length;
}

void simx_callback_tcp_msg(sim_con_status_t con_status, uint8_t n)
{
    g_con_status = con_status;
    g_con_n = n;
}

void simx_callback_tcp_data(uint8_t *data, uint16_t length, uint8_t n)
{
    g_tcp_data = (char*)data;
    g_tcp_length = length;
    g_tcp_con = n;
}

void simx_callback_pdp_deact()
{
    is_deact = 1;
}

void simx_test_send(const char* msg)
{
    for(int i = 0; i < strlen(msg); i++)
    {
        simx_receive(msg[i]);
    }
}

TEST(AutoTestSim900, SIM_AT)
{
    EXPECT_EQ(1, 1);
    ASSERT_THAT(0, Eq(0));
    
    sim_reply_t reply;
    simx_test(&reply);
    
    EXPECT_EQ(g_length, 4);
    EXPECT_STREQ(g_data, "AT\r\n");
    
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    
    simx_test(&reply);
    simx_test_send("\r\nERROR\r\n");
    EXPECT_EQ(reply.status, SIM300_ERROR);
    
    simx_test(&reply);
    simx_test_send("\r\nFDSG\r\n");
    EXPECT_EQ(reply.status, SIM300_ERRFRAME);
}

TEST(AutoTestSim900, SIM_AT_CPIN)
{
    sim_reply_t reply;
    simx_pin_is_required(&reply);
    
    EXPECT_EQ(g_length, strlen("AT+CPIN?\r\n"));
    EXPECT_STREQ(g_data, "AT+CPIN?\r\n");
    
    simx_test_send("\r\n+CPIN: READY\r\n\r\n");
    simx_test_send("OK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    EXPECT_EQ(sim_pin_required(), PIN_READY);
    
    
    static char *msg[] = {"READY", "SIM PIN", "SIM PUK", "PH_SIM PIN", 
                          "PH_SIM PUK", "SIM PIN2", "SIM PUK2"};
    for(int i = 0; i < sizeof(msg) / sizeof(msg[0]); i++)
    {
        simx_pin_is_required(&reply);
        simx_test_send("\r\n+CPIN: ");
        simx_test_send(msg[i]);
        simx_test_send("\r\n\r\n");
        simx_test_send("OK\r\n");
        EXPECT_EQ(reply.status, SIM300_OK);
        EXPECT_EQ(sim_pin_required(), i);
    }
    
    simx_pin_is_required(&reply);
    simx_test_send("\r\n+CPIN: NO_VALID\r\n");;
    simx_test_send("OK\r\n");
    EXPECT_EQ(simx_is_receive(), 0);
    //EXPECT_EQ(reply.status, SIM300_ERRFRAME);
    //EXPECT_EQ(pin_required(), SIM_PIN_UNKNOW);
    
    simx_pin_is_required(&reply);
    simx_test_send("\r\n+CPIN: NO_VALID\r\n\r\n");;
    simx_test_send("OK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    EXPECT_EQ(sim_pin_required(), SIM_PIN_UNKNOW);
    
}

TEST(AutoTestSim900, SIM_AT_CREG)
{
    sim_reply_t reply;
    simx_network_registration(&reply);
    EXPECT_EQ(g_length, strlen("AT+CREG?\r\n"));
    EXPECT_STREQ(g_data, "AT+CREG?\r\n");
    
    simx_test_send("\r\n+CREG: 0,1\r\n\r\n");
    simx_test_send("OK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    EXPECT_EQ(sim_network_registration(), NET_REGISTERED_HOME_NETWORK);
    
    for(int i = 0; i < 5; i++)
    {
        simx_network_registration(&reply);
        simx_test_send("\r\n+CREG: 0,");
        char c[2] = {(char)i + 0x30, 0};
        simx_test_send(c);
        simx_test_send("\r\n\r\n");
        simx_test_send("OK\r\n");
        EXPECT_EQ(reply.status, SIM300_OK);
        EXPECT_EQ(sim_network_registration(), i);
    }
}

TEST(AutoTestSim900, SIM_AT_CMGF)
{
    sim_reply_t reply;
    simx_sms_mode(&reply, SIM_SMS_TEXT);
    EXPECT_EQ(g_length, strlen("AT+CMGF=1\r\n"));
    EXPECT_STREQ(g_data, "AT+CMGF=1\r\n");
    
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    
    simx_sms_mode(&reply, SIM_SMS_PDU);
    EXPECT_EQ(g_length, strlen("AT+CMGF=0\r\n"));
    EXPECT_STREQ(g_data, "AT+CMGF=0\r\n");
    
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
}

TEST(AutoTestSim900, SIM_AT_CMGS)
{
    sim_reply_t reply;
    simx_send_sms(&reply, "+796400011133", "test");
    EXPECT_EQ(g_length, strlen("AT+CMGS=\"+796400011133\"\r\n"));
    EXPECT_STREQ(g_data, "AT+CMGS=\"+796400011133\"\r\n");
    simx_test_send("\r\n> ");
    EXPECT_EQ(g_length, strlen("test\x1A"));
    EXPECT_STREQ(g_data, "test\x1A");
    
    simx_test_send("\r\n+CMGS: 232\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
}

TEST(AutoTestSim900, SIM_AT_CSMINS)
{
    sim_reply_t reply;
    simx_sim_inserted_status(&reply);
    EXPECT_EQ(g_length, strlen("AT+CSMINS?\r\n"));
    EXPECT_STREQ(g_data, "AT+CSMINS?\r\n");
    simx_test_send("\r\n+CSMINS: 0,1\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(sim_is_inserted(), 1);
    EXPECT_EQ(reply.status, SIM300_OK);
    
    simx_sim_inserted_status(&reply);
    simx_test_send("\r\n+CSMINS: 0,0\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(sim_is_inserted(), 0);
    
    simx_sim_inserted_status(&reply);
    simx_test_send("\r\n+CSMINS: 0,44\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(sim_is_inserted(), -1);   
}

TEST(AutoTestSim900, SIM_AT_CGATT)
{
    sim_reply_t reply;
    simx_is_attach_to_GPRS(&reply);
    EXPECT_EQ(g_length, strlen("AT+CGATT?\r\n"));
    EXPECT_STREQ(g_data, "AT+CGATT?\r\n");
    simx_test_send("\r\n+CGATT: 1\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(gprs_is_attach(), 1);
    EXPECT_EQ(reply.status, SIM300_OK);
    
    simx_is_attach_to_GPRS(&reply);
    EXPECT_EQ(g_length, strlen("AT+CGATT?\r\n"));
    EXPECT_STREQ(g_data, "AT+CGATT?\r\n");
    simx_test_send("\r\n+CGATT: 0\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(gprs_is_attach(), 0);
    EXPECT_EQ(reply.status, SIM300_OK);
    
    simx_is_attach_to_GPRS(&reply);
    EXPECT_EQ(g_length, strlen("AT+CGATT?\r\n"));
    EXPECT_STREQ(g_data, "AT+CGATT?\r\n");
    simx_test_send("\r\n+CGATT: 22\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(gprs_is_attach(), -1);
    EXPECT_EQ(reply.status, SIM300_OK);
}

TEST(AutoTestSim900, SIM_AT_CIPSHUT)
{
    sim_reply_t reply;
    simx_deactivate_gprs_pdp(&reply);
    EXPECT_EQ(g_length, strlen("AT+CIPSHUT\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPSHUT\r\n");
    simx_test_send("\r\nSHUT OK\r\n");
    EXPECT_EQ(reply.status, SIM300_SHUT_OK);
    
    simx_deactivate_gprs_pdp(&reply);
    simx_test_send("\r\nERROR\r\n");
    EXPECT_EQ(reply.status, SIM300_ERROR);
    
    simx_deactivate_gprs_pdp(&reply);
    simx_test_send("\r\ntete\r\n");
    EXPECT_EQ(reply.status, SIM300_ERRFRAME);
    
    simx_deactivate_gprs_pdp(&reply);
    simx_test_send("tete\r\n");
    EXPECT_EQ(reply.status, SIM300_ERRFRAME);
}

TEST(AutoTestSim900, SIM_AT_CIPSTATUS)
{
    sim_reply_t reply;
    simx_current_connection_status(&reply);
    EXPECT_EQ(g_length, strlen("AT+CIPSTATUS\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPSTATUS\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    simx_test_send("\r\nSTATE: IP INITIAL\r\n\r\n");
    EXPECT_EQ(current_connection_status(), CIP_IP_INITIAL);
    
    simx_current_connection_status(&reply);
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    simx_test_send("\r\nSTATE: IP START\r\n\r\n");
    EXPECT_EQ(current_connection_status(), CIP_IP_START);
    
    simx_current_connection_status(&reply);
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    simx_test_send("\r\nSTATE: PDP DEACT\r\n\r\n");
    EXPECT_EQ(current_connection_status(), CIP_PDP_DEACT);
}

TEST(AutoTestSim900, SIM_AT_CIPMUX)
{
    sim_reply_t reply;
    simx_multiple_connection(&reply, 0);
    EXPECT_EQ(g_length, strlen("AT+CIPMUX=0\r\n"));
    EXPECT_EQ(strlen(g_data), strlen("AT+CIPMUX=0\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPMUX=0\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    EXPECT_EQ(sim_cip_mux_mode(), 0);
    
    simx_multiple_connection(&reply, 1);
    EXPECT_STREQ(g_data, "AT+CIPMUX=1\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    EXPECT_EQ(sim_cip_mux_mode(), 1);
}

TEST(AutoTestSim900, SIM_AT_CSTT)
{
    sim_reply_t reply;
    simx_set_gprs_config(&reply, "internet.mts.ru", "mts", "mts");
    EXPECT_EQ(g_length, strlen("AT+CSTT=\"internet.mts.ru\",\"mts\",\"mts\"\r\n"));
    EXPECT_STREQ(g_data, "AT+CSTT=\"internet.mts.ru\",\"mts\",\"mts\"\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
}

TEST(AutoTestSim900, SIM_AT_CIICR)
{
    sim_reply_t reply;
    simx_bring_up_wireless_connection(&reply);
    EXPECT_EQ(g_length, strlen("AT+CIICR\r\n"));
    EXPECT_STREQ(g_data, "AT+CIICR\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
}

TEST(AutoTestSim900, SIM_AT_CIFSR)
{
    sim_reply_t reply;
    sim_ip_t ip;
    simx_get_local_ip(&reply, &ip);
    EXPECT_EQ(g_length, strlen("AT+CIFSR\r\n"));
    EXPECT_STREQ(g_data, "AT+CIFSR\r\n");
    simx_test_send("\r\n192.168.0.1\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    EXPECT_EQ(ip.addr0, 192);
    EXPECT_EQ(ip.addr1, 168);
    EXPECT_EQ(ip.addr2, 0);
    EXPECT_EQ(ip.addr3, 1);
}

TEST(AutoTestSim900, SIM_AT_CSQ)
{
    sim_reply_t reply;
    uint8_t lvl = -1;
    simx_signal_quality_report(&reply, &lvl);
    EXPECT_EQ(g_length, strlen("AT+CSQ\r\n"));
    EXPECT_STREQ(g_data, "AT+CSQ\r\n");
    simx_test_send("\r\n+CSQ: 31,0\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    EXPECT_EQ(lvl, 31);
    
}

TEST(AutoTestSim900, SIM_AT_CIPSTART)
{
    sim_reply_t reply;
    
    simx_tcp_connect(&reply, SIM_TCP, "www.google.com", 80, 1);
    EXPECT_EQ(g_length, strlen("AT+CIPSTART=1,\"TCP\",\"www.google.com\",80\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPSTART=1,\"TCP\",\"www.google.com\",80\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    
    
}

TEST(AutoTestSim900, SIM_AT_CIPSEND)
{
    sim_reply_t reply;
    simx_multiple_connection(&reply, 0);
    simx_test_send("\r\nOK\r\n");
    
    char msg[] = "test";
    int n = strlen(msg);
    simx_tcp_send_data(&reply, (uint8_t*)msg, n, 0);
    EXPECT_EQ(g_length, strlen("AT+CIPSEND=4\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPSEND=4\r\n");
    simx_test_send("> ");
    EXPECT_EQ(g_length, n);
    EXPECT_STREQ(g_data, msg);
    simx_test_send("\r\nSEND OK\r\n");
    EXPECT_EQ(reply.status, SIM300_SEND_OK);
    
    simx_tcp_send_data(&reply, (uint8_t*)msg, n, 0);
    EXPECT_EQ(g_length, strlen("AT+CIPSEND=4\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPSEND=4\r\n");
    simx_test_send("> ");
    EXPECT_EQ(g_length, n);
    EXPECT_STREQ(g_data, msg);
    simx_test_send("\r\nSEND FAIL\r\n");
    EXPECT_EQ(reply.status, SIM300_SEND_FAIL);
    
    simx_multiple_connection(&reply, 1);
    simx_test_send("\r\nOK\r\n");
    
    simx_tcp_send_data(&reply, (uint8_t*)msg, n, 1);
    EXPECT_EQ(g_length, strlen("AT+CIPSEND=1,4\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPSEND=1,4\r\n");
    simx_test_send("> ");
    EXPECT_EQ(g_length, n);
    EXPECT_STREQ(g_data, msg);
    simx_test_send("\r\n1, SEND OK\r\n");
    EXPECT_EQ(reply.status, SIM300_SEND_OK);
    
    simx_tcp_send_data(&reply, (uint8_t*)msg, n, 2);
    EXPECT_EQ(g_length, strlen("AT+CIPSEND=2,4\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPSEND=2,4\r\n");
    simx_test_send("> ");
    EXPECT_EQ(g_length, n);
    EXPECT_STREQ(g_data, msg);
    simx_test_send("\r\n2, SEND FAIL\r\n");
    EXPECT_EQ(reply.status, SIM300_SEND_FAIL);
    
}

TEST(AutoTestSim900, SIM_AT_CIPCLOSE)
{
    sim_reply_t reply;
    simx_multiple_connection(&reply, 0);
    simx_test_send("\r\nOK\r\n");
    
    simx_tcp_close(&reply, 0);
    EXPECT_EQ(g_length, strlen("AT+CIPCLOSE=0\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPCLOSE=0\r\n");
    simx_test_send("\r\nCLOSE OK\r\n");
    EXPECT_EQ(reply.status, SIM300_CLOSE_OK);
    
    simx_tcp_close(&reply, 0);
    EXPECT_EQ(g_length, strlen("AT+CIPCLOSE=0\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPCLOSE=0\r\n");
    simx_test_send("\r\nERROR\r\n");
    EXPECT_EQ(reply.status, SIM300_ERROR);
    
    simx_multiple_connection(&reply, 1);
    simx_test_send("\r\nOK\r\n");
    
    simx_tcp_close(&reply, 1);
    EXPECT_EQ(g_length, strlen("AT+CIPCLOSE=1,0\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPCLOSE=1,0\r\n");
    simx_test_send("\r\n1, CLOSE OK\r\n");
    EXPECT_EQ(reply.status, SIM300_CLOSE_OK);
    
    simx_tcp_close(&reply, 2);
    EXPECT_EQ(g_length, strlen("AT+CIPCLOSE=2,0\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPCLOSE=2,0\r\n");
    simx_test_send("\r\nERROR\r\n");
    EXPECT_EQ(reply.status, SIM300_ERROR);
}

TEST(AutoTestSim900, SIM_AT_CIPHEAD)
{
    sim_reply_t reply;
    simx_tcp_head_enable(&reply, 0);
    EXPECT_EQ(g_length, strlen("AT+CIPHEAD=0\r\n"));
    EXPECT_EQ(strlen(g_data), strlen("AT+CIPHEAD=0\r\n"));
    EXPECT_STREQ(g_data, "AT+CIPHEAD=0\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    //EXPECT_EQ(sim_cip_mux_mode(), 0);
    
    simx_tcp_head_enable(&reply, 1);
    EXPECT_STREQ(g_data, "AT+CIPHEAD=1\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    //EXPECT_EQ(sim_cip_mux_mode(), 1);
}

TEST(AutoTestSim900, SIM_AT_CSCS)
{
    sim_reply_t reply;
    
    simx_set_TE_character(&reply, SIM_IRA);
    EXPECT_EQ(strlen(g_data), strlen("AT+CSCS=\"IRA\"\r\n"));
    EXPECT_EQ(g_length, strlen("AT+CSCS=\"IRA\"\r\n"));
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
}

TEST(AutoTestSim900MSG, SIM_MSG_CALLBACK)
{
    sim_reply_t reply;
    
    simx_multiple_connection(&reply, 0);
    simx_test_send("\r\nOK\r\n");
    
    simx_test_send("\r\nALREADY CONNECT\r\n");
    EXPECT_EQ(g_con_status, SIM_TCP_ALREADY_CONNECT);
    simx_test_send("\r\nCLOSED\r\n");
    EXPECT_EQ(g_con_status, SIM_TCP_CLOSED);
    simx_test_send("\r\nCONNECT OK\r\n");
    EXPECT_EQ(g_con_status, SIM_TCP_CONNECT_OK);
    simx_test_send("\r\nCONNECT FAIL\r\n");
    EXPECT_EQ(g_con_status, SIM_TCP_CONNECT_FAIL);
    
    simx_multiple_connection(&reply, 1);
    EXPECT_STREQ(g_data, "AT+CIPMUX=1\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    EXPECT_EQ(sim_cip_mux_mode(), 1);
    
    simx_test_send("\r\n1, ALREADY CONNECT\r\n");
    EXPECT_EQ(g_con_status, SIM_TCP_ALREADY_CONNECT);
    EXPECT_EQ(g_con_n, 1);
    simx_test_send("\r\n2, CLOSED\r\n");
    EXPECT_EQ(g_con_status, SIM_TCP_CLOSED);
    EXPECT_EQ(g_con_n, 2);
    simx_test_send("\r\n3, CONNECT OK\r\n");
    EXPECT_EQ(g_con_status, SIM_TCP_CONNECT_OK);
    EXPECT_EQ(g_con_n, 3);
    simx_test_send("\r\n4, CONNECT FAIL\r\n");
    EXPECT_EQ(g_con_status, SIM_TCP_CONNECT_FAIL);
    EXPECT_EQ(g_con_n, 4);
    
}

TEST(AutoTestSim900MSG, SIM_TCP_CALLBACK)
{
    sim_reply_t reply;
    
    simx_multiple_connection(&reply, 1);
    simx_test_send("\r\nOK\r\n");
    
    simx_test_send("\r\n+RECEIVE,1,11:\r\nSOMETHING\r\n");
    EXPECT_STREQ(g_tcp_data, "SOMETHING\r\n");
    EXPECT_EQ(g_tcp_length, 11);
    EXPECT_EQ(g_tcp_con, 1);
    
    /***********************************************/
    simx_sms_mode(&reply, SIM_SMS_TEXT);
    EXPECT_EQ(g_length, strlen("AT+CMGF=1\r\n"));
    EXPECT_STREQ(g_data, "AT+CMGF=1\r\n");

    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    
    simx_sms_mode(&reply, SIM_SMS_PDU);
    EXPECT_EQ(g_length, strlen("AT+CMGF=0\r\n"));
    EXPECT_STREQ(g_data, "AT+CMGF=0\r\n");
    
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    
    /***********************************************/
    
    simx_multiple_connection(&reply, 1);
    simx_test_send("\r\nOK\r\n");
    
    simx_tcp_head_enable(&reply, 1);
    EXPECT_STREQ(g_data, "AT+CIPHEAD=1\r\n");
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    
    simx_test_send("\r\n+IPD,11:\r\nSOMETHING\r\n");
    EXPECT_STREQ(g_tcp_data, "SOMETHING\r\n");
    EXPECT_EQ(g_tcp_length, 11);
    
    g_tcp_data = NULL;
    g_tcp_length = 0;
    simx_test_send("\r\nSOMETHING\r\n");
    //EXPECT_EQ(g_tcp_data, NULL);
    EXPECT_EQ(g_tcp_length, 0);
    
    /***********************************************/
    simx_sms_mode(&reply, SIM_SMS_TEXT);
    EXPECT_EQ(g_length, strlen("AT+CMGF=1\r\n"));
    EXPECT_STREQ(g_data, "AT+CMGF=1\r\n");

    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    
    simx_sms_mode(&reply, SIM_SMS_PDU);
    EXPECT_EQ(g_length, strlen("AT+CMGF=0\r\n"));
    EXPECT_STREQ(g_data, "AT+CMGF=0\r\n");
    
    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    
    /***********************************************/
    
}

TEST(AutoTestSim900MSG, SIM_SMS_CALLBACK)
{
    sim_reply_t reply;
    simx_test_send("\r\n+CMTI: \"SM\",4\r\n");
    EXPECT_EQ(g_sms_number, 4);
    /**********************************************/
    simx_sms_mode(&reply, SIM_SMS_TEXT);
    EXPECT_EQ(g_length, strlen("AT+CMGF=1\r\n"));
    EXPECT_STREQ(g_data, "AT+CMGF=1\r\n");

    simx_test_send("\r\nOK\r\n");
    EXPECT_EQ(reply.status, SIM300_OK);
    /**********************************************/
    
}

TEST(AutoTestSim900MSG, SIM_PDP_CALLBACK)
{
    
    simx_test_send("\r\n+PDP: DEACT\r\n");
    EXPECT_EQ(is_deact, 1);
}

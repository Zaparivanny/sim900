[![Build Status](https://travis-ci.org/Zaparivanny/sim900.svg?branch=develop)](https://travis-ci.org/Zaparivanny/sim900)
[![Coverage Status](https://coveralls.io/repos/github/Zaparivanny/sim900/badge.svg)](https://coveralls.io/github/Zaparivanny/sim900)

# sim900 library


## How to build this library

```sh
git clone --recursive https://github.com/Zaparivanny/sim900.git

```

## Supported commands

| COMMAND  |  description |
|----------|-------------:|
| AT                                                                   |
| AT+CPIN?     | Enter PIN                                             |
| AT+CREG?     | Network Registration                                  |
| AT+CMGF=     | Select SMS Message Format                             |
| AT+CMGS=     | Send sms message                                      |
| AT+CSMINS?   | Sim inserted status reporting                         |
| AT+CGATT?    | Attach or detach from gprs service                    |
| AT+CIPSHUT   | Deactivate gprs pdp context                           |
| AT+CIPSTATUS | Query Current Connection Status                       |
| AT+CIPMUX=   | Start Up Multi-IP Connection                          |
| AT+CSTT=     | Start Task and Set APN, USER NAME, PASSWORD           |
| AT+CIICR     | Bring Up Wireless Connection with GPRS or CSD         |
| AT+CIFSR     | Get Local IP Address                                  |
| AT+CSQ       | Signal quality report                                 |
| AT+CIPSTART= | Start up tcp or udp connection                        |
| AT+CIPSEND=  | Send data through tcp or udp connection               |
| AT+CIPCLOSE= | Close tcp or udp connection                           |
| AT+CIPHEAD=  | Add an ip head at the beginning of a package received |
| AT+CSCS=     | Select TE Character Set                               |

## Examples
* Connect to gprs

```cpp
    sim_reply_t reply;
    simx_is_attach_to_GPRS(&reply);
    simx_wait_reply();
    if(reply.status != SIM300_OK){return;}
    
    simx_deactivate_gprs_pdp(&reply);
    simx_wait_reply();
    if(reply.status != SIM300_SHUT_OK){return;}
    
    simx_current_connection_status(&reply);
    simx_wait_reply();
    if(reply.status != SIM300_OK){return;}
    
    simx_set_gprs_config(&reply, APN, APN_USER_NAME, APN_PASSWORD);
    simx_wait_reply();
    if(reply.status != SIM300_OK){return;}
    
    simx_bring_up_wireless_connection(&reply);
    simx_wait_reply();
    if(reply.status != SIM300_OK){return;}
    
    sim_ip_t ip;
    simx_get_local_ip(&reply, &ip);
    simx_wait_reply();
    if(reply.status != SIM300_OK){return;}
    std::cout << "IP:" << (int)ip.addr0 << "." << (int)ip.addr1 << "."
                       << (int)ip.addr2 << "."<< (int)ip.addr3 << "." << std::endl;
```

* Send SMS
```cpp
    sim_reply_t reply;
    simx_send_sms(&reply, PHONE_NUMBER, "test");
    simx_wait_reply();
    simx_wait_reply();
```

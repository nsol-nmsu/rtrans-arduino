#include "xbee_init.h"

bool xbee_wait_for_ok(SoftwareSerial &xs){
        const char okcr[] = "OK\r";
        bool err = false;

        /* wait for 3 bytes */
        while(xs.available() < 3);

        /* verify that it was an OK */
        for(int i = 0; i < 3; i++){
                char c = xs.read();
                if(c != okcr[i]){
                        err = true;
                }
        }

        return !err;
}

bool xbee_command(SoftwareSerial &xs, const char command[]){
        /* Clear garbage from the buffer */
        while(xs.available())
                xs.read();
                
        /* Write the command */
        xs.write(command);
        
        return xbee_wait_for_ok(xs);
}

bool xbee_init(SoftwareSerial &xs){
        bool ok = true;
        const char *commands[] = {
                "+++",
                "ATRE\r",
                "ATFR\r",
                "+++",
                "ATCH0C\r",
                "ATID3332\r",
                "ATAP2\r",
                "ATAC\r",
                "ATCN\r"
        };
        int i;
        for(i = 0; i < 3; i++){
                ok &= xbee_command(xs, commands[i]);
        }
        delay(1000);
        for(i = 3; i < 9; i++){
                ok &= xbee_command(xs, commands[i]);
        }
        delay(1000);
        return ok;
}

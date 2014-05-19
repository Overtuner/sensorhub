/*
sensorhub.cpp
A unix-deamon to handle and store the information from/to all connected sensornodes. 
All information is stored in a SQLite3 database.

Version history:
V0.1: 
Initial Version
Node sends its information activ to the master
Master is only receiver
V0.2:
Changed the delivery
Master takes control over everything
Node wakes up in a defined interval and listens into the network for something to do
Database structure changed - not comüatible with V0.1 
V0.3:
Small changes in database structure
Added Web-GUI (German only)
V0.4:
Database structure changed and extended - not compatible with V0.3 
Added actors







*/
#define DEBUG 1
#define DBNAME "/var/database/sensorhub.db"
#define LOGFILE "/var/log/sensorhubd.log"
#define ERRSTR "ERROR: "
#define DEBUGSTR "Debug: "

//--------- End of global define -----------------


#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string> 
#include <RF24.h>
#include <RF24Network.h>
#include <time.h>
#include <stdio.h>
#include <sqlite3.h>

using namespace std;

// Setup for GPIO 25 CE and CE0 CSN with SPI Speed @ 8Mhz
RF24 radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_1MHZ);  
RF24Network network(radio);

// Structure of our payload
struct payload_t {
  uint16_t Job;
  uint16_t seq; 
  float value;
};
payload_t payload;

// Structure to handle the orderqueue
struct order_t {
  uint16_t Job;
  uint16_t seq; 
  uint16_t to_node; 
  unsigned char channel; 
  float value;
};

int orderloopcount=0;
int ordersqlexeccount=0;
bool ordersqlrefresh=true;

sqlite3 *db;
sqlite3_stmt *stmt;   

RF24NetworkHeader rxheader;
RF24NetworkHeader txheader;

char buffer1[50];
char buffer2[50];
char err_prepare[]=ERRSTR "Could not prepare SQL statement";
char err_execute[]=ERRSTR "Could not execute SQL statement";
char err_opendb[]=ERRSTR "Opening database: " DBNAME " failed !!!!!";
char msg_startup[]="Startup sensorhubd";

bool logmsg(char *mymsg){
  FILE * pFile;
  bool exitcode=true; 
  char buf[3];
  pFile = fopen (LOGFILE,"a");
  if (pFile!=NULL) {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    fprintf (pFile, "Sensorhubd: %d.", ltm->tm_year + 1900 );
    if ( ltm->tm_mon + 1 < 10) sprintf(buf,"0%d",ltm->tm_mon + 1); else sprintf(buf,"%d",ltm->tm_mon + 1);
    fprintf (pFile, "%s.", buf );
    if ( ltm->tm_mday < 10) sprintf(buf,"0%d",ltm->tm_mday); else sprintf(buf,"%d",ltm->tm_mday);
    fprintf (pFile, "%s ", buf );
    if ( ltm->tm_hour < 10) sprintf(buf," %d",ltm->tm_hour); else sprintf(buf,"%d",ltm->tm_hour);
    fprintf (pFile, "%s:", buf );
    if ( ltm->tm_min < 10) sprintf(buf,"0%d",ltm->tm_min); else sprintf(buf,"%d",ltm->tm_min);
    fprintf (pFile, "%s:", buf );
    if ( ltm->tm_sec < 10) sprintf(buf,"0%d",ltm->tm_sec); else sprintf(buf,"%d",ltm->tm_sec);
    fprintf (pFile, "%s : %s \n", buf, mymsg );
    fclose (pFile);
  } else {
    exitcode = false;
  }
  return exitcode;
}

void log_sqlite3_errstr(int dbrc){
  char retval[80];    
  FILE * pFile;
  pFile = fopen (LOGFILE,"a");
  switch (dbrc) {
    case 1:
      sprintf(retval,"SQLITE_ERROR:1: SQL error or missing database");
      break;
    case 2:
      sprintf(retval,"SQLITE_INTERNAL:2: internal logic error in SQLite");
      break;
    case 3:
      sprintf(retval,"SQLITE_PERM:3: Access permission denied");
      break;
    case 4:
      sprintf(retval,"SQLITE_ABORT:4: Callback routine requested an abort");
      break;
    case 5:
      sprintf(retval,"SQLITE_BUSY:5: The database file is locked");
      break;
    case 6:
      sprintf(retval,"SQLITE_LOCKED:6: A table in the database is locked");
      break;
    case 7:
      sprintf(retval,"SQLITE_NOMEM:7: A malloc() failed");
      break;
    case 8:
      sprintf(retval,"SQLITE_READONLY:8: Attempt to write a readonly database");
      break;
    case 9:
      sprintf(retval,"SQLITE_INTERRUPT:9: Operation terminated by sqlite_interrupt()");
      break;
    case 10:
      sprintf(retval,"SQLITE_IOERR:10: Some kind of disk I/O error occurred");
      break;
    case 11:
      sprintf(retval,"SQLITE_CORRUPT:11: The database disk image is malformed");
      break;
    case 12:
      sprintf(retval,"SQLITE_NOTFOUND:12: (Internal Only) Table or record not found");
      break;
    case 13:
      sprintf(retval,"SQLITE_FULL:13: Insertion failed because database is full");
      break;
    case 14:
      sprintf(retval,"SQLITE_CANTOPEN:14: Unable to open the database file");
      break;
    case 15:
      sprintf(retval,"SQLITE_PROTOCOL:15: Database lock protocol error");
      break;
    case 16:
      sprintf(retval,"SQLITE_EMPTY:16: (Internal Only) Database table is empty");
      break;
    case 17:
      sprintf(retval,"SQLITE_SCHEMA:17: The database schema changed");
      break;
    case 18:
      sprintf(retval,"SQLITE_TOOBIG:18: Too much data for one row of a table");
      break;
    case 19:
      sprintf(retval,"SQLITE_CONSTRAINT:19:: Abort due to constraint violation");
      break;
    case 20:
      sprintf(retval,"SQLITE_MISMATCH:20: Data type mismatch");
      break;
    case 21:
      sprintf(retval,"SQLITE_MISUSE:21: Library used incorrectly");
      break;
    case 22:
      sprintf(retval,"SQLITE_NOLFS:22: Uses OS features not supported on host");
      break;
    case 23:
      sprintf(retval,"SQLITE_AUTH:23: Authorization denied");
//    #define SQLITE_ROW         100  /* sqlite_step() has another row ready */
//    #define SQLITE_DONE        101  /* sqlite_step() has finished executing */
      break;
    default:
      sprintf(retval,"Unknown Error from SQLITE");
  }
  fprintf (pFile, "SQLite: %s \n", retval );
  fclose (pFile);
}

void do_sql(char *mysql) {
  int rc;
  rc = sqlite3_prepare(db, mysql, -1, &stmt, 0 ); 
  if ( rc != SQLITE_OK) {
    logmsg(err_prepare);
    logmsg(mysql);
    log_sqlite3_errstr(rc);
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    logmsg(err_execute);
    logmsg(mysql);
    log_sqlite3_errstr(rc);
  }
  rc=sqlite3_finalize(stmt);
  if ( rc != SQLITE_OK) {
    logmsg(err_prepare);
    logmsg(mysql);
    log_sqlite3_errstr(rc);
  }
}

void del_messageentry(uint16_t Job, uint16_t seq) {
  char sql_stmt[300];            
  sprintf(sql_stmt, " delete from messagebuffer where Job = %u and seq = %u "
                  , Job, seq );
  do_sql(sql_stmt);
#ifdef DEBUG 
  logmsg(sql_stmt);
#endif
  ordersqlrefresh=true;
}

void check_trigger(float last_val, float akt_val, int sensor) {
  int edge; // 0=> falling; 1=> rising
  if ( akt_val >= last_val) { edge = 1;  } else { edge = 0;  }  
  // check if there is a trigger for this case
  
  
}

void store_value(uint16_t job, uint16_t seq, float val) {
  char sql_stmt[300];
  float last_val;
  int sensor;
  sprintf(sql_stmt,"select max(utime), sensor, value from sensordata where sensor = (select id from job where job = %u and seq = %u )",job, seq);              
  int rc = sqlite3_prepare(db, sql_stmt, -1, &stmt, 0 ); 
  if ( rc != SQLITE_OK) {
    logmsg(err_prepare);
    logmsg(sql_stmt);
    log_sqlite3_errstr(rc);
  }
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    last_val = sqlite3_column_double (stmt, 2);
    sensor  = sqlite3_column_int (stmt, 1);
  }
  rc=sqlite3_finalize(stmt);
  if ( rc != SQLITE_OK) {
    logmsg(err_prepare);
    logmsg(sql_stmt);
    log_sqlite3_errstr(rc);
  }
  check_trigger(last_val, val, sensor);
  sprintf(sql_stmt,"insert or replace into sensordata (sensor, utime, value) "
                   "select id, "
                   "strftime('%%s', datetime('now','localtime')), %f "
                   " from job "
                   "where job = %u and seq = %u ", val, job, seq);
#ifdef DEBUG 
  logmsg(sql_stmt);
#endif
  do_sql(sql_stmt);
  sprintf(sql_stmt,"update sensor set last_value = %f, last_TS = datetime('now', 'localtime') "
                   " where sensor = (select id from job where job = %u and seq = %u and type=1) ", val, job, seq);
  do_sql(sql_stmt);
#ifdef DEBUG 
  logmsg(sql_stmt);
#endif
}

int getnodeadr(char *node) {
  int mynodeadr = 0;
  bool err = false;
  char t[5];
  for ( int i = 0; (node[i] > 0) && (! err); i++ ) {
    if ( mynodeadr > 0 ) mynodeadr = (mynodeadr << 3);
    sprintf(t,"%c",node[i]); 
    mynodeadr = mynodeadr + atoi(t);
    err = (node[i] == '6' || node[i] == '7' || node[i] == '8' || node[i] == '9' || (( i > 0 ) && ( node[i] == '0' ))); 
  }
  if (err) mynodeadr = 0;
  return mynodeadr;
}

int getparentnodeadr(int nodeadr) {
  int parentnodeadr=-1;
  if ( nodeadr > 0x7FFF ) parentnodeadr = 0;
  else if ( nodeadr > 7*8*8*8 ) parentnodeadr = (nodeadr & 0x0FFF);
  else if ( nodeadr > 7*8*8 ) parentnodeadr = (nodeadr & 0x01FF);
  else if ( nodeadr > 7*8 ) parentnodeadr = (nodeadr & 0x003F);
  else if ( nodeadr > 7 ) parentnodeadr = (nodeadr & 0x0007);
  else parentnodeadr = 0;
  return parentnodeadr;
}


int main(int argc, char** argv) {
	order_t order[7]; // we do not handle more than 6 orders (one per subnode 1...6) at one time
	for (int i=1; i<7; i++) { // init order array
		order[i].Job = 0;
		order[i].seq = 0;
		order[i].to_node  = ' ';
		order[i].channel  = 0;
		order[i].value = 0;
	}
	if (! logmsg(msg_startup)) {
		printf("sensorhubd: Error opening logfile: %s \n",LOGFILE);
		return 1;
	}
  radio.begin();
  delay(5);
  network.begin( 90, 0);
  radio.setDataRate(RF24_250KBPS);
#ifdef DEBUG 
  radio.printDetails();
#endif        
  char sql_stmt[300];
  int rc = sqlite3_open("/var/database/sensorhub.db", &db);
  if (rc) {
    logmsg (err_opendb);
  } else {
    // Start Cleanup
    sprintf (sql_stmt, "delete from messagebuffer "); 
#ifdef DEBUG 
    logmsg(sql_stmt);        
#endif        
    do_sql(sql_stmt);
    // End Cleanup	
    long int time_old=0;
    while(1) {
      network.update();
      if ( network.available() ) {
//
// Receive loop: react on the message from the nodes
//
        network.read(rxheader,&payload,sizeof(payload));
#ifdef DEBUG 
        char debug[300];
        sprintf(debug, DEBUGSTR "Received: Channel: %u from Node: %o to Node: %o Job %d Seq %d Value %f "
                     , rxheader.type, rxheader.from_node, rxheader.to_node, payload.Job, payload.seq, payload.value);
        logmsg(debug);
#endif        
        uint16_t sendernode=rxheader.from_node;
        switch (rxheader.type) {

          case 1 ... 20: {
            // Sensor 
#ifdef DEBUG 
            sprintf(debug, DEBUGSTR "Value of sensor %u on Node: %o is %f ", rxheader.type, sendernode, payload.value);
            logmsg(debug);        
#endif        
            del_messageentry(payload.Job, payload.seq);  
            store_value(payload.Job, payload.seq, payload.value); 
            break; }

          case 21 ... 99: {
            // Actor 
#ifdef DEBUG 
            sprintf(debug, DEBUGSTR "Value of sensor %u on Node: %o is %f ", rxheader.type, sendernode, payload.value);
            logmsg(debug);        
#endif        
            del_messageentry(payload.Job, payload.seq);  
//            store_value(payload.Job, payload.seq, payload.value); 
            break; }

          case 101: {
            // battery volatage
#ifdef DEBUG 
            sprintf(debug, DEBUGSTR "Voltage of Node: %o is %f ", sendernode, payload.value);
            logmsg(debug);        
#endif        
            del_messageentry(payload.Job, payload.seq);  
            store_value(payload.Job, payload.seq, payload.value); 
            sprintf(sql_stmt,"update node set Battery_act = %f where node = '0%o'", payload.value, sendernode);
            do_sql(sql_stmt);
#ifdef DEBUG 
            logmsg(sql_stmt);
#endif
            break; }
          case 111: {
#ifdef DEBUG 
            sprintf(debug, DEBUGSTR "Init of Node: %o finished: Sleeptime set to %f ", sendernode, payload.value);
            logmsg(debug);        
#endif        
            del_messageentry(payload.Job, payload.seq);  
            break; }
          case 112: {
            txheader.type=112;
            bool radio_always_on = (payload.value >0.5);
            network.write(txheader,&payload,sizeof(payload));
#ifdef DEBUG 
            if ( radio_always_on ) sprintf(debug, "---Debug: Radio allways on for Node: %o finished.", sendernode);
            else sprintf(debug, "---Debug: Radio allways off for Node: %o finished.", sendernode);
            logmsg(debug);        
#endif        
            del_messageentry(payload.Job, payload.seq);  
            break;  }              
          case 117: {
#ifdef DEBUG 
            sprintf(debug, DEBUGSTR "Wake up of Node: %o finished.", sendernode);
            logmsg(debug);        
#endif        
            del_messageentry(payload.Job, payload.seq);  

            break; }
          case 119: {
            // Init sequence may be changed individuelly by order
            sprintf (sql_stmt, "select sleeptime, radiomode from node where node = '0%o' LIMIT 5 ",sendernode);
            rc = sqlite3_prepare(db, sql_stmt, -1, &stmt, 0 ); 
            if ( rc != SQLITE_OK) {
              logmsg(err_prepare);
              logmsg(sql_stmt);
              log_sqlite3_errstr(rc);
            }
 			int sleeptime=60000; // set defaults
			int radiomode=0;
            while (sqlite3_step(stmt) == SQLITE_ROW) {
              sleeptime = sqlite3_column_int (stmt, 0);
              radiomode = sqlite3_column_int (stmt, 1);			
			}
            uint16_t sendernode=rxheader.from_node;
            payload.value = sleeptime;
            RF24NetworkHeader txheader(sendernode,111);
            if (network.write(txheader,&payload,sizeof(payload)) ) {
#ifdef DEBUG 
            sprintf(debug, DEBUGSTR "Send to Node: %o Channel %u Value %f", sendernode, txheader.type, payload.value);
            logmsg(debug);    
            } else {
            sprintf(debug, DEBUGSTR "Failed: Send to Node: %o Channel %u Value %f", sendernode, txheader.type, payload.value);
            logmsg(debug);    			
#endif        
            delay(100);
			}
            txheader.type=112;
            payload.value = radiomode;
            if (network.write(txheader,&payload,sizeof(payload)) ) {
#ifdef DEBUG 
            sprintf(debug, DEBUGSTR "Send to Node: %o Channel %u Value %f", sendernode, txheader.type, payload.value);
            logmsg(debug);        
            } else {
            sprintf(debug, DEBUGSTR "Failed: Send to Node: %o Channel %u Value %f", sendernode, txheader.type, payload.value);
            logmsg(debug);    			
#endif        
            delay(100);
			}
            txheader.type=119;
            if (network.write(txheader,&payload,sizeof(payload))) {
#ifdef DEBUG 
            sprintf(debug, DEBUGSTR "Send to Node: %o Channel %u Value %f", sendernode, txheader.type, payload.value);
            logmsg(debug);        
            } else {
            sprintf(debug, DEBUGSTR "Failed: Send to Node: %o Channel %u Value %f", sendernode, txheader.type, payload.value);
            logmsg(debug);    			
#endif        
            }
            break; }
        }
        orderloopcount=0;
	  } // network.available
//
// Dispatcher: Look if the is anything to schedule
//
      if ( time(0) > time_old + 59 ) {  // check every minute if we have jobs to schedule
        time_old = time(0);
//
// Cleanup old jobs that have not been executed during the last 10 minutes
//
        sprintf (sql_stmt, "delete from messagebuffer"
                 " where strftime('%%s','now') - utime > 600 " );
#ifdef DEBUG 
        logmsg(sql_stmt);        
#endif        
        do_sql(sql_stmt);
//
// Case 1: Jobs that run  immeadeately (start = -1) and run only once (interval = -1)
//
        sprintf (sql_stmt, "insert into Scheduled_Jobs (job)"
                 " select job from schedule "
                 "  where start = '-1' and interval = -1 ");
#ifdef DEBUG 
        logmsg(sql_stmt);        
#endif        
        do_sql(sql_stmt);
        sprintf (sql_stmt, "delete from schedule where start = '-1' and interval = -1 "); 
#ifdef DEBUG 
        logmsg(sql_stmt);        
#endif        
        do_sql(sql_stmt);
//
// Case 2: Jobs that run at a scheduled time (start) and run only once (interval = -1)
//
        sprintf (sql_stmt, "insert into Scheduled_Jobs (job)"
                 " select job from schedule "
                 "  where datetime(start) <= datetime('now','localtime') and interval = -1 ");
#ifdef DEBUG 
        logmsg(sql_stmt);        
#endif        
        do_sql(sql_stmt);
        sprintf (sql_stmt, "delete from schedule where start != '-1' and datetime(start) <= datetime('now','localtime') and interval = -1 "); 
#ifdef DEBUG 
        logmsg(sql_stmt);        
#endif        
        do_sql(sql_stmt);
//
// Case 3: Jobs that start immeadeately (start = -1) and run every <interval> minutes
// 
        sprintf (sql_stmt, "insert into Scheduled_Jobs (job)"
                 " select job from schedule "
                 " where start = '-1' and interval > 0 ");
#ifdef DEBUG 
        logmsg(sql_stmt);        
#endif        
        do_sql(sql_stmt);
        sprintf (sql_stmt, "update schedule set start = datetime('now', 'localtime', '+'||interval||' minutes') where start = '-1' and interval > 0 "); 
#ifdef DEBUG 
        logmsg(sql_stmt);        
#endif        
        do_sql(sql_stmt);
//
// Case 4: Jobs that run frequently - increment "start" by "interval" minutes 
//
        sprintf (sql_stmt, "insert into Scheduled_Jobs (job)"
                 " select job from schedule"
                 "  where datetime(start) <= datetime('now','localtime') and interval > 0 ");
#ifdef DEBUG 
        logmsg(sql_stmt);        
#endif        
        do_sql(sql_stmt);
        sprintf (sql_stmt, "update schedule set start = datetime(start, '+'||interval||' minutes') "
                           "where datetime(start) <= datetime('now','localtime') and interval > 0 "); 
#ifdef DEBUG 
        logmsg(sql_stmt);        
#endif        
        do_sql(sql_stmt);
// Put all Jobentries into messagebuffer
        sprintf (sql_stmt, "insert into messagebuffer (job,seq,node,channel,value,utime)"
                 "  select job, seq, node, channel, value,strftime('%%s','now') from Scheduled_messages ");
#ifdef DEBUG 
        logmsg(sql_stmt);        
#endif        
        do_sql(sql_stmt);
// Delete all entries in Scheduled_Jobs, we dont need them any more
        sprintf (sql_stmt, " delete from Scheduled_Jobs ");
#ifdef DEBUG 
        logmsg(sql_stmt);        
#endif        
        do_sql(sql_stmt);

		
		
//
// End Dispatcher
//
        orderloopcount=0;
        ordersqlrefresh=true;
      }
//
// Orderloop: Tell the nodes what they have to do
//
			if ( orderloopcount == 0 ) {
				if (ordersqlexeccount > 30 || ordersqlrefresh) {
					for (int i=0; i<7; i++) {
						sprintf (sql_stmt, "select job, seq, node, channel, value from messagebuffer where substr(Node,length(node),1) = '%d' order by CAST(node as integer) LIMIT 1 ",i);
						rc = sqlite3_prepare(db, sql_stmt, -1, &stmt, 0 ); 
						if ( rc != SQLITE_OK) {
							logmsg(err_prepare);
							logmsg(sql_stmt);
							log_sqlite3_errstr(rc);
						}
						order[i].Job = 0;
						if(sqlite3_step(stmt) == SQLITE_ROW) {
							order[i].Job = sqlite3_column_int (stmt, 0);
							order[i].seq = sqlite3_column_int (stmt, 1);
							char nodebuf[10];
							sprintf(nodebuf,"%s",sqlite3_column_text (stmt, 2));
							order[i].to_node  = getnodeadr(nodebuf);
							order[i].channel  = sqlite3_column_int (stmt, 3);
							order[i].value = sqlite3_column_double (stmt, 4);
						}
						rc=sqlite3_finalize(stmt);
						if ( rc != SQLITE_OK) {
							logmsg(err_prepare);
							logmsg(sql_stmt);
							log_sqlite3_errstr(rc);
						}
						ordersqlexeccount=0;
						ordersqlrefresh=false;
					}
				}
				ordersqlexeccount++;
				int i=1;
				while (i<7) {
					if (order[i].Job) {
						txheader.from_node = 0;
						payload.Job = order[i].Job;
						payload.seq = order[i].seq;
						txheader.to_node  = order[i].to_node;
						txheader.type  = order[i].channel;
						payload.value = order[i].value;
						if (network.write(txheader,&payload,sizeof(payload))) {
#ifdef DEBUG 
							char debug[300];
							sprintf(debug, DEBUGSTR "Send: Channel: %u from Node: %o to Node: %o Job %d Seq %d Value %f "
									, txheader.type, txheader.from_node, txheader.to_node, payload.Job, payload.seq, payload.value);
							logmsg(debug); 
						} else {		
							char debug[300];
							sprintf(debug, DEBUGSTR "Failed: Send: Channel: %u from Node: %o to Node: %o Job %d Seq %d Value %f "
									, txheader.type, txheader.from_node, txheader.to_node, payload.Job, payload.seq, payload.value);
							logmsg(debug); 
#endif      
						}  
					}
					i++; 
				}
			} // /orderloopcount
			orderloopcount++;
			if ( orderloopcount > 200 ) {  orderloopcount=0; }
			usleep(1000); 
//
//  end orderloop 
//
      if (! (order[1].Job || order[2].Job || order[3].Job || order[4].Job || order[5].Job || order[6].Job)) usleep(100000);
		} // while(1)
	} 
	return 0;
}



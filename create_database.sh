#!/bin/sh

DB=sensorhub.db

if [ -e $DB ]; then
  rm $DB
fi  
sqlite3 $DB <<EOF
CREATE TABLE Node
       (Node TEXT PRIMARY KEY, Battery_UN FLOAT, Battery_UE FLOAT, Location TEXT, Battery_act FLOAT, Nodename TEXT);
CREATE TABLE job
       ( jobno INT, seq INT, Sensor INT, value FLOAT,
       PRIMARY KEY ( jobno, seq ),
       FOREIGN KEY (Sensor) REFERENCES Sensor(Sensor) );
CREATE TABLE jobchain
       (jobno INT, jobdesc TEXT);
CREATE TABLE messagebuffer
       ( jobno INT, seq INT, node TEXT, channel INT, value FLOAT,
       PRIMARY KEY ( jobno, seq ));
CREATE TABLE schedule
       (schedule INT PRIMARY KEY,jobno int, Start, Interval INT,
       FOREIGN KEY (Jobno) REFERENCES jobchain(JOBNO));
CREATE TABLE sensor
       (Sensor INT PRIMARY KEY, Sensorinfo TEXT, Node TEXT, Channel INT, Last_Value FLOAT, Last_TS TEXT,
       FOREIGN KEY (Node) REFERENCES Node(Node) );
CREATE TABLE sensordata
       (Sensor INT, Year INT, Month INT, Day INT, Hour INT, Value FLOAT,
       PRIMARY KEY (Year, Month, Day, Hour, Sensor));
CREATE VIEW message2send
       as select jobno, min(seq) as aseq, node, channel, value from messagebuffer
       group by jobno;
CREATE VIEW job_v as
       select  a.jobno, a.seq, b.node, b.channel, a.value
       from job a, sensor b
       where a.sensor = b.sensor;
EOF




echo "Database: $DB created"

if [ -e populate_DB.sql ]; then
  sqlite3 $DB < populate_DB.sql
  echo  "Database: $DB filled with data from populate_DB.sql"
fi

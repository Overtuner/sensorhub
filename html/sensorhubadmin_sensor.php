<?php
#*******************************************************************
#
# Funktionen zur Sensorpflege
#
#*******************************************************************

function input_select_node($mynode) {
  global $db;
  echo "<select name='node' id='node' size=1 class='input'>";
  if ($mynode == "") { echo "<option value='' selected>---</option>"; }
  $results = $db->query("SELECT Node, Nodename from node");
  while ($row = $results->fetchArray()) {
  switch ($row[0]) {
    case $mynode:
      echo "<option value='",$row[0],"' selected>",$row[1],"</option>";
    break;
    default:
      echo "<option value='",$row[0],"' >",$row[1],"</option>";
    }
  }
  echo "</select>";
}

function list_sensor() {
  global $db;
  global $tabledetails;
  echo "<center><table $tabledetails>";
  echo "<tr><th>Sensor</th><th>Node</th><th>Channel</th><th>Letzter Wert</th><th>Zeitpunkt</th><th>&nbsp;</th></tr>";
  $results = $db->query("SELECT Sensor, Sensorinfo, Node, Channel, Last_Value, Last_TS, julianday(date('now')) - julianday(date(last_ts)) from sensor order by substr(Node,length(node),1), substr(Node,length(node)-1,1), substr(Node,length(node)-2,1) ");
  while ($row = $results->fetchArray()) {
  if ($row[6] > 1) { $td_col = "bgcolor=#999999"; } else { $td_col = ""; }
   echo "<tr class=block2><td $td_col>",$row[1],"</td><td $td_col align=right>",$row[2],"</td><td $td_col>",$row[3],"</td><td $td_col>",$row[4],"</td><td $td_col>",$row[5],"</td><td $td_col>",
        "<button class=myButton value=$row[0] onclick=getresult(this.value,'sensor','edit_sensor');>Editieren</button>",
         "<button class=myButton value=$row[0] onclick=getresult(this.value,'sensor','delete_sensor');>L&ouml;schen</button>",
         "</td></tr>\n";
  }
  echo "<tr><td colspan=8 align=right>&nbsp;",
       "<button class=myButton value=0 onclick=getresult(this.value,'sensor','new_sensor');>Neuen Sensor anlegen</button>",
       "&nbsp;</td></tr>",
       "</table></center>\n";
}

function edit_sensor($mysensor) {
  global $db;
  global $tabledetails_edit;
  echo "<center><table $tabledetails_edit><tr><th colspan=2>";
  if ($mysensor==0) {
    $results = $db->query("SELECT ifnull(max(Sensor),0)+1 from sensor ");
    $row = $results->fetchArray();
	$mysensor=$row[0];
    $mysensorinfo="";
    $mynode="";
    $mychannel="";
    echo "Anlegen eines neuen Sensors ($mysensor)";
  } else {
    $results = $db->query("SELECT Sensor, Sensorinfo, Node, Channel from sensor where sensor = $mysensor ");
    $row = $results->fetchArray();
    $mysensorinfo=$row[1];
    $mynode=$row[2];
    $mychannel=$row[3];
    echo "Editieren eines Sensors ($mysensor)";
  }
  echo 	"</th></tr>",
        "<tr class=block2><td>Sensorname:</td><td>",
        "<input type=hidden id=sensor value=$mysensor>",
		"<input class=input id=sensorinfo value='",$mysensorinfo,"'>","</td></tr>",
		"<tr><td>Node:</td><td>";
  input_select_node($mynode);
  echo "</td></tr>",
       "<tr><td>Channel:</td><td><input class=input id=channel value='",$mychannel,"'>","</td></tr>",
       "<tr><td colspan=2><center>&nbsp;";
  if ($mysensor==0) {
    echo "<button class=myButton onclick=savenewsensor();>Sensor anlegen</button>";
  } else {
    echo "<button class=myButton onclick=updatesensor();>Werte eintragen</button>";
  }
  echo "<button class=myButton onclick=listsensor();>Abbruch</button>",
       "&nbsp;</center></td></tr>";
  echo "</table>",
       "</center>";
}

function update_sensor($mysensor) {
  global $db;
  $mysensorinfo=$_GET["sensorinfo"];
  $mynode=$_GET["node"];
  $mychannel=$_GET["channel"];
  $sql="update sensor set node = '$mynode', channel = $mychannel, sensorinfo = '$mysensorinfo' where sensor = $mysensor ";
  if ( $db->exec($sql) ) {
    mymessage("info","gespeichert");
  } else {
    mymessage("error","Fehler: ",$db->lastErrorMsg()," <br>SQL: $sql");
  }
}

function savenew_sensor($mysensor) {
  global $db;
  $mysensorinfo=$_GET["sensorinfo"];
  $mynode=$_GET["node"];
  $mychannel=$_GET["channel"];
  $sql="insert into sensor (sensor, sensorinfo, node, channel) values( $mysensor,'$mysensorinfo', '$mynode', $mychannel) ";
  if ( $db->exec($sql) ) {
    mymessage("info","gespeichert");
  } else {
    mymessage("error","Fehler: ",$db->lastErrorMsg()," <br>SQL: $sql");
  }
}

function delete_sensor($mysensor) {
  global $db;
  $sql="select count(*) from job where sensor = $mysensor ";
  $results = $db->query($sql);
  $row = $results->fetchArray();
  if ( $row[0] > 0 ) {
      mymessage("error","Fehler: Jobdatens�tze vorhanden, dieser Sensor/Aktor kann nicht gel�scht werden!");
  } else {
    $sql="delete from sensor where sensor = $mysensor ";
    if ( $db->exec($sql) ) {
      mymessage("info","gespeichert");
    } else {
      mymessage("error","Fehler: ",$db->lastErrorMsg()," <br>SQL: $sql");
    }
  }
}
?>
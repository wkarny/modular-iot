<?php  session_start(); 
  $page = $_SERVER['PHP_SELF'];
  $sec = "2";
?>
<meta http-equiv="refresh" content="<?php echo $sec?>;URL='<?php echo $page?>'">

<?php
      if(!isset($_SESSION['user']))
      {
           header("Location:login.php");  
       }
       include_once "includes/header.php";

          echo "Hello : ".$_SESSION['user'];

          echo '<a align="right" href="logout.php"> Logout</a> '; 

        $host=$_SESSION['host'];
	    $port=$_SESSION['port'];
        $rid = rand(10,500);
        $message = 'GATQ+'.$rid;
		// create socket
		$socket = socket_create(AF_INET, SOCK_STREAM, 0) or die("Could not create socket\n");
		// connect to server
		$result = socket_connect($socket, $host, $port) or die("Could not connect to server\n");  
		// send string to server
		socket_write($socket, $message, strlen($message)) or die("Could not send data to server\n");
		// get server response
		echo '<br/><h2>Current Sensor Network</h2><br/>';
		echo '<table border=1>';
		echo '<tr><td>ID</td><td>Type</td><td>Current Value</td></tr>';
		$result='';
		while($result != "GATR+END+".$rid){
			$result = socket_read ($socket, 1024) or die("Could not read server response\n");
			$token=explode("+",$result);
			if(sizeof($token)==5){
				echo '<tr>';
				if($token[2]=="0")
					echo '<td><a href="led.php?tid='.$token[1].'">'.$token[1].'</a></td>';
				else
					echo '<td>'.$token[1].'</td>';
				for($i=2;$i<4;$i+=1)
					echo '<td>'.$token[$i].'</td>';
				echo '</tr>';
		    }

	    }
	    echo '</table>';
	    socket_close($socket);
?>

<?php
include_once "includes/footer.php";
?>
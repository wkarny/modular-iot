<?php  session_start(); ?>
<?php
      if(!isset($_SESSION['user']))
      {
           header("Location:login.php");  
       }
       include_once "includes/header.php";

          echo "Hello : ".$_SESSION['user'];

          echo '<a align="right" href="logout.php"> Logout</a> '; 
        $tid=$_GET['tid'];
        $host=$_SESSION['host'];
	    $port=$_SESSION['port'];
        $rid = rand(10,500);
        $message = 'GTUQ+'.$tid.'+'.$rid;
		// create socket
		$socket = socket_create(AF_INET, SOCK_STREAM, 0) or die("Could not create socket\n");
		// connect to server
		$result = socket_connect($socket, $host, $port) or die("Could not connect to server\n");  
		// send string to server
		socket_write($socket, $message, strlen($message)) or die("Could not send data to server\n");
		// get server response
		$result = socket_read ($socket, 1024) or die("Could not read server response\n");
		$token=explode("+",$result);
		echo '<h2>The status of LED:</h2>';
		if(sizeof($token)==4){
			$tdata=$token[2];
			if($tdata == "255"){
				echo "The LED is ON<br/><br/>";
			}
			else{
				echo "The LED is OFF<br/><br/>";
			}
		}
		else{
			echo "Error is parsing / TID not found";
		}
	    socket_close($socket);

	    if(isset($_POST['switch']))  {
	    	$rid = rand(10,500);
	    	$temp_data=0;
	    	if($tdata==255)
	    		$temp_data=0;
	    	else
	    		$temp_data=255;
	    	$message = 'PTUQ+'.$tid.'+'.$temp_data.'+'.$rid;
			// create socket
			$socket = socket_create(AF_INET, SOCK_STREAM, 0) or die("Could not create socket\n");
			// connect to server
			$result = socket_connect($socket, $host, $port) or die("Could not connect to server\n");  
			// send string to server
			socket_write($socket, $message, strlen($message)) or die("Could not send data to server\n");
			// get server response
			$result = socket_read ($socket, 1024) or die("Could not read server response\n");
			socket_close($socket);
			$token=explode("+",$result);
			if($token[2]=="ACK"){
				header("Location:led.php?tid=".$tid);  
			}
			else{
				header("Location:led.php?tid=".$tid);  // should be modified
				//echo "Switching Failed";
			}
	    }

?>

<form action="" method="post">
     <input type="submit" name="switch" value="Switch">
</form>

<?php
include_once "includes/footer.php";
?>
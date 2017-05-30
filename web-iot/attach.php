<?php  session_start(); ?>

<?php
      if(!isset($_SESSION['user']))
      {
           header("Location:login.php");  
       }

     if(isset($_POST['attach']))  {
	     $uid = $_POST['uid'];
	     $nid = $_POST['nid'];
	     $host=$_SESSION['host'];
	     $port=$_SESSION['port'];
	     $rid = rand(10,500);

		$message = 'ADQ+'.$nid.'+'.$uid.'+'.$rid;
		// create socket
		$socket = socket_create(AF_INET, SOCK_STREAM, 0) or die("Could not create socket\n");
		// connect to server
		$result = socket_connect($socket, $host, $port) or die("Could not connect to server\n");  
		// send string to server
		socket_write($socket, $message, strlen($message)) or die("Could not send data to server\n");
		// get server response
		$result = socket_read ($socket, 1024) or die("Could not read server response\n");
		//echo "Reply From Server  :".$result;
		// close socket
		socket_close($socket);
		$token=array();
		$tok = strtok($result, "+");
		for($i=0;$i<3;$i+=1){
			if($tok == false)
				echo "Error in parsing";
			else{
				$token[$i]=$tok;
				$tok = strtok("+");
			}
		}

		if($token[0]=="ADR"){
			if($token[2] == "$rid"){
			    if($token[1]=='0')
			    	echo "Device Addition Failed : Device not added";
			    else{
			    	echo '<script language="javascript">';
					echo 'alert("Device Successfully Added")';
					echo '</script>';
			    }
		    }
		    else{
		    	echo "Got worng ReqID: ".$token[2];
		    }
		}
		else{
			echo "Expecting ADR, but got : ".$token[0];
		}

     }
?>

<?php
include_once "includes/header.php";
echo "Hello : ".$_SESSION['user'];
?>

<h2>Add Device</h2>

<form action="" method="post">

    <table width="200" border="0">
  <tr>
    <td>  Device Unique ID</td>
    <td> <input type="text" name="uid" > </td>
  </tr>
  	<tr>
	    <td> Prefered Node ID</td>
	    <td> <input type="text" name="nid" > </td>
    </tr>
    <tr>
    <td> <input type="submit" name="attach" value="Attach"></td>
    <td></td>
  </tr>
</table>
</form>

<?php
include_once "includes/footer.php";
?>
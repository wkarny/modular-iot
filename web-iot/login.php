<?php  session_start(); ?>


<?php

if(isset($_SESSION['user']))   
 {
    header("Location:index.php"); 
 }

if(isset($_POST['login']))  
{
     $user = $_POST['user'];
     $pass = $_POST['pass'];


    $host    = $_POST['ip'];
	$port    = $_POST['port'];
	$message = 'LOGIN+'.$user.'+'.$pass;
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

      if($result == "LOGIN+ACK")   
         {                                     

          $_SESSION['user']=$user;
          $_SESSION['host']=$host;
          $_SESSION['port']=$port;


         echo '<script type="text/javascript"> window.open("index.php","_self");</script>'; 

        }

        else
        {
            echo '<script language="javascript">';
            echo 'alert("Invalid username or password")';
            echo '</script>';    
        }
}
 ?>

<?php
include_once "includes/header.php";
?>

<h2>Modular IoT</h2>

<form action="" method="post">

    <table width="200" border="0">
  <tr>
    <td>  Username</td>
    <td> <input type="text" name="user" > </td>
  </tr>
  <tr>
    <td> Password  </td>
    <td><input type="password" name="pass"></td>
  </tr>
  <tr>
  	<tr>
    <td>  IP</td>
    <td> <input type="text" name="ip" > </td>
    </tr>
    <tr>
    <td>  PORT</td>
    <td> <input type="text" name="port" > </td>
    </tr>
    <td> <input type="submit" name="login" value="LOGIN"></td>
    <td></td>
  </tr>
</table>
</form>

<?php
include_once "includes/footer.php";
?>
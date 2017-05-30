<?php
 session_start();

  echo "Logout Successful ";
  session_destroy();   // function that Destroys Session 
  header("Location: login.php");
?>
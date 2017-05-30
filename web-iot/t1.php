<?php
include_once "includes/header.php";
echo '<script src="jscolor.js"></script>';
$string = "This is\tan example\nstring";
/* Use tab and newline as tokenizing characters as well  */
$tok = strtok($string, " \n\t");

while ($tok !== false) {
    echo "Word=$tok<br />";
    $tok = strtok(" \n\t");
}
$var=123;
if("123"=="$var")
	echo "Yes";
?>

<?php include_once "includes/footer.php"; ?>
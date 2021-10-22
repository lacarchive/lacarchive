<?
	$_POST['item'] = $_SERVER['HTTP_REFERER'];
	mail('info.lacarchive@gmail.com', 'New bid from ' . $_POST['full_name'], print_r($_POST, true));
	header('Location: thankyou.html');
?>
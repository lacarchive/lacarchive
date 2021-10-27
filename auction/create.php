<?
	$items_string = file_get_contents('items.json');
	$items_string = substr($items_string, 14);
	$length = strlen($items_string) - 2;
	$items_string = substr($items_string, 0, $length);
	$items = json_decode($items_string);

	$new_item = new stdClass();
	$king = 0;
	foreach ($items as $item) if ($item->id > $king) $king = $item->id;
	$new_item->title = $_POST['title'];
	$new_item->artist = $_POST['artist'];
	$new_item->year = $_POST['year'];
	$new_item->medium = $_POST['medium'];
	$new_item->dimensions = $_POST['dimensions'];
	$new_item->price = $_POST['price'];
	$new_item->opening_bid = $_POST['opening_bid'];
	$new_item->id = $king + 1;
	$items[] = $new_item;

	file_put_contents('items.json', "items_data = '" . json_encode($items) . "';");
	header('Location: index.html');
?>
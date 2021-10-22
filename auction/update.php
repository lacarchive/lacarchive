<?
	$items_string = file_get_contents('items.json');
	$items_string = substr($items_string, 14);
	$length = strlen($items_string) - 2;
	$items_string = substr($items_string, 0, $length);
	$items = json_decode($items_string);

	$new_item = new stdClass();
	$new_item->id = $_POST['id'];
	$new_item->title = $_POST['title'];
	$new_item->artist = $_POST['artist'];
	$new_item->year = $_POST['year'];
	$new_item->medium = $_POST['medium'];
	$new_item->dimensions = $_POST['dimensions'];
	$new_item->price = $_POST['price'];
	$new_item->opening_bid = $_POST['opening_bid'];

	foreach($items as $key => $item)
	{
		if ($item->id == $new_item->id) 
		{ 
			$items[$key] = $new_item;
			break;
		}
	}
	
	$items = (array) $items;
	

	file_put_contents('items.json', "items_data = '" . json_encode(array_values($items)) . "';");
	header('Location: ' . $_SERVER['HTTP_REFERER']);
?>
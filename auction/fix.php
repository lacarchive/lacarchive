<?
	$items_string = file_get_contents('items.json');
	$items_string = substr($items_string, 14);
	$length = strlen($items_string) - 2;
	$items_string = substr($items_string, 0, $length);
	$items = json_decode($items_string);

	function cmp($a, $b)
	{
	    return strcmp($a->artist, $b->artist);
	}

	foreach($items as $key => $item)
	{
		if ($item->id == $_GET['id'])
		{
			$delete_id = $item->id;
			$delete_key = $key;
			break;
		} 
	}

	//unset($items[$key]);

	usort($items, "cmp");
	$items = (array) $items;
	
	file_put_contents('items.json', "items_data = '" . json_encode(array_values($items)) . "';");
	header('Location: index.html');
?>
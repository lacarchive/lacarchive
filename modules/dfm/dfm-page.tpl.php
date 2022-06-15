<!DOCTYPE html>
<html lang="<?php print $GLOBALS['language']->language; ?>">

<head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
  <meta name="robots" content="noindex,nofollow" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title><?php print $page['title']; ?></title>
  <?php foreach ($page['cssUrls'] as $url): ?>
    <link type="text/css" rel="stylesheet" media="all" href="<?php print $url; ?>" />
  <?php endforeach; ?>
  <?php foreach ($page['jsUrls'] as $url): ?>
    <script type="text/javascript" src="<?php print $url; ?>"></script>
  <?php endforeach; ?>
  <?php print $page['head']; ?>
</head>

<body class="dfm">
<script type="text/javascript">
  dfm.load(document.body, <?php print json_encode($page['scriptConf']); ?>);
</script>
</body>

</html>

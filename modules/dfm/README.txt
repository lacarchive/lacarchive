Drupella File Manager
====================================


FEATURES
-----------
  - AJAX powered drag & drop user interface
  - File operations: Upload, Delete, Move, Copy, Rename
  - Folder operations: Create, Delete, Move, Copy, Rename
  - Image operations: Resize, Crop
  - Mouse indicators for move-copy permissions during drag & drop.
  - Multiple file upload
  - Upload progress
  - Drag drop files from local computer
  - File sorting by name, size, date, type
  - Keyboard shortcuts for file operations similar to common file explorers of Windows and Linux.
  - Switching between icon view and list view
  - Small footprint with overall 90kb javascript. (30kb gzipped)
  - Very fast javascript DOM rendering. A folder containing 10000 files takes only a few seconds to render.
  - Configuration profiles per user roles
  - Configurable limits: upload file size, disk quota, file extensions, image dimensions
  - Multiple personal or shared folders for users
  - Token support in folder names
  - File permissions per folder
  - Support for inline image & file link insertion into textareas
  - Display thumbnails as image icons using image styles
  - Integration with WYSIWYG module, CKEditor module, and BUEditor
  - Custom wysiwyg plugins for inserting multiple images or files into editor content
  - Support for private file system
  - Allow FTP uploaded files in file/image fields using the included dfm_filefield module
  - Support for custom stream wrappers like Amazon S3
  - Search files and folders


NOTE: In this documentation DFM URL is given as /dfm. 
If your Drupal is working under a subdirectory like http://domain/drupal7 then DFM URL should be /drupal7/dfm


INSTALLATION
-----------
  1) Copy the dfm module directory to sites/all/modules
  2) Enable the module in Drupal admin interface
  3) Create configuration profiles and assign them to user roles at /admin/config/media/dfm
  4) You can set a thumbnail style for image icons in profile configuration under "Advanced configuration" -> "Show image icons as thumbnails"
  5) Users can access the file manager at /dfm. Non default file systems can be accessed at dfm/SCHEME
  6) Enable "Drupella FM for File Field" module if you want to select files from Drupella FM for your image/file fields.
      You must check the "Allow users to select files from Drupella FM for this field" option in your field's configuration page.

Note1: If you are upgrading from DFM Lite just copy the full version over the old module folder.
Note2: If you are using Amazon S3 please apply the patch at https://www.drupal.org/node/2222005


WYSIWYG Module Integration
-----------
  1) Edit your wysiwyg editor profile at /admin/config/content/wysiwyg
  2) Enable "Drupella FM: Integrate into image/link dialogs" under "Buttons and plugins" section
  3) Save
  4) Now the file browser should appear when you click browse button in image/link dialogs.

Note: There are also individual buttons that can be enabled for inserting multiple files without image/link dialogs
  - "Drupella FM: Button for inserting multiple images"
  - "Drupella FM: Button for inserting multiple file links"

 
CKEditor Module Integration
-----------
  1) Edit your ckeditor profile at /admin/config/content/ckeditor
  2) Select "none" for "File browser settings" -> "File browser type"
  3) Set "Advanced options" -> "Custom JavaScript configuration" field as
      config.filebrowserBrowseUrl = '/dfm?fileHandler=dfmWysiwyg.ckeHandler';
  4) Save
  5) Now the file browser should appear when you click browse button in image/link dialogs.

Note: There are also individual buttons that can be enabled for inserting multiple files without image/link dialogs
  - You first need to enable "Drupella FM: Buttons for inserting multiple images and file links" under "Editor appearance" > "Plugins"
  - Then you can enable buttons "Insert images" and "Insert files" under "Toolbar"


BUEditor Module Integration
-----------
  1) Edit your editor at /admin/config/content/bueditor. You can work on a copy of default editor.
  2) Add a new button (This will load DFM integration library and will not be visible)
      Title: DFM integration
      Content: php: module_invoke('dfm', 'wysiwyg_integrate');
  3) The default image/link buttons have IMCE integration which can be replaced by Drupella FM.
     Replace E.imce.button('attr_src') text with (window.dfmWysiwyg ? dfmWysiwyg.bueButton('attr_src') : '') in image button
     Replace E.imce.button('attr_href') text with (window.dfmWysiwyg ? dfmWysiwyg.bueButton('attr_href') : '') in link button
  4) Save and test the demo at the bottom of the page
  
Note: You can also create individual buttons for inserting multiple files without image/link dialogs
  - Title=Insert images
    Content=js: if (window.dfmWysiwyg) dfmWysiwyg.bueBrowser(E, 'image');
  - Title=Insert files
    Content=js: if (window.dfmWysiwyg) dfmWysiwyg.bueBrowser(E, 'file');
 
 
Textarea(plain) Integration
-----------
  1) Enter your textarea ids into "Common settings" -> "Enable image/link insertion into textareas" at /admin/config/media/dfm.
  2) Now you should see "Insert image" and "Insert file link" links under your textareas.

Standalone Usage
-----------
  1) Go to example.com/dfm to manage files in your default file system.
  2) For non default file systems go to example.com/dfm/SCHEME where SCHEME is the identifier of the file file system. Ex: example.com/dfm/private


Custom Application Integration
-----------
  1) Define a file handler function for the file manager to call. Ex:
      window.myFileHandler = function(file, fmWindow) {
        // Get file properties
        var url = file.getUrl(),
        name = file.name,
        size = file.size,
        date = file.date,
        safeName = file.formatName(),
        formatSize = file.formatSize(),
        formatDate = file.formatDate();
        // Do something using the properties
        // Close the file manager window
        fmWindow.close();
      }
  2) Open the file manager in a new window/iframe with the url /dfm?fileHandler=myFileHandler
  3) myFileHandler will be called by the file manager when the user selects a file.
  4) The url can also include initHandler=myInitHandler which allows to run a function on UI initiation.


EMBEDDING INTO A CUSTOM PAGE
-----------
  - Other than directly accessing the menu path /dfm, you can embed the file manager into a page body in a few ways

  1) Iframe with Full HTML input format
      <iframe src="/dfm" class="dfm-iframe" frameborder="0" style="border: 1px solid #eee; width: 99%; height: 540px"></iframe>

  2) Popup link with Full HTML input format
      <a href="#" onclick="window.open('/dfm', '', 'width=800,height=540,resizable=1'); return false;">Open File Manager</a>

  3) Direct UI with PHP input format
      <?php
      // Check access
      if (module_invoke('dfm', 'access')) {
        $url = url('dfm');
        $libpath = dfm_script_path('core/misc');
        drupal_add_css("$libpath/dfm.css");
        drupal_add_js("$libpath/dfm.js");
        drupal_add_js("jQuery(function(){dfm.load('dfm-wrapper', {url: '$url'})});", 'inline');
      }
      ?>
      <div id="dfm-wrapper"></div>
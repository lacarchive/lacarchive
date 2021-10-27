/**
 * @file
 * Javascript to enable client side uploading of files to S3.
 */

(function ($) {
  AmazonS3corsUpload = {};
  Drupal.behaviors.amazonS3corsUpload = {};

  AmazonS3corsUpload.handleUpload = function(file_selector, $form, triggering_element) {
    // Retrieve the file object.
    $file = $(file_selector);
    f = $file[0].files[0];
    // And the form_build_id which we need to lookup the form during our AJAX
    // request.
    form_build_id = $form.find('input[name="form_build_id"]').val();
    if (typeof f != 'undefined') {
      // Add a placholder for our progress bar.
      $file.hide().after('<div id="amazons3-cors-progress" style="width: 270px; float: left;">' + Drupal.t('Preparing upload ...') + '</div>');

      // Use the file object and ask Drupal to generate the appropriate signed
      // request for us.
      var postData = {
        filename: f.name,
        filesize: f.size,
        filemime: f.type,
        triggering_element: triggering_element,
        form_build_id: form_build_id
      };

      // POST to Drupal which will return the required paramaters for signing
      // a CORS request.
      $.post(Drupal.settings.basePath + 'ajax/amazons3_cors', postData, function(data) {
        // Take the signed data and construct a form out of it.
        var fd = new FormData();
        $.each(data.inputs, function(key, value) {
          fd.append(key, value);
        });

        // Add the file to be uploaded.
        fd.append('file', f);

        // Construct our AJAX request paramaters.
        var params = {
          url: 'http://' + data.form.action + '/',
          processData: false,
          data: fd,
          type: 'POST',
          cache: false,
          contentType: false,
          mimeType: 'multipart/form-data',
          // This works with jQuery 1.5.1+, however the withCredentials doesn't
          // stick w/ older versions. So we handle it in the beforeSend method.
          xhrFields: {
            withCredentials: true
          },
          // This can be removed when Drupal upgrades to a version of jQuery
          // that is > 1.5.1.
          beforeSend: function(xhr) {
            xhr.withCredentials = true;
          },
          xhr: function() {
            myXhr = $.ajaxSettings.xhr();
            if(myXhr.upload){
              $file.hide();
              $('#amazons3-cors-progress').html('');
              myXhr.upload.addEventListener('progress', (function(e) {
                return AmazonS3corsUpload.displayProgress($file, e);
              }), false);
            }
            return myXhr;
          },
          error: function() {
            // todo: deal w/ upload errors.
            console.log(arguments);
          },
          complete: function() {
            // Update the hidden fields to tell Drupal about the file that
            // was just uploaded.
            $file.parent().find('input[name$="[filemime]"]').val(f.type);
            $file.parent().find('input[name$="[filesize]"]').val(f.size);
            // Make sure and use the filename provided by Drupal as it may have
            // been renamed.
            $file.parent().find('input[name$="[filename]"]').val(data.file_real);
            // Re-enable all the submit buttons.
            $form.find('input[type="submit"]').removeAttr('disabled');
            // Find trigger the #ajax method for the upload button that was
            // initially clicked to upload the file.
            var button_id = $file.parent().find('input.cors-form-submit').attr('id');
            ajax = Drupal.ajax[button_id];
            ajax.form.ajaxSubmit(ajax.options);
          }
        };

        $.ajax(params);
      });
    }
    else {
      $form.submit();
    }
  }

  /**
   * Receives an XMLHttpRequestProgressEvent and uses it to display current
   * progress if possible.
   *
   * @param event
   *   And XMLHttpRequestProgressEvent object.
   */
  AmazonS3corsUpload.displayProgress = function(el, event) {
    if (event.lengthComputable) {
      percent = Math.floor((event.loaded / event.total) * 100);
      $('#amazons3-cors-progress').progressbar({value: percent});
      return true;
    }
  }

  /**
   * Implements Drupal.behaviors.
   */
  Drupal.behaviors.amazonS3corsUpload.attach = function(context, settings) {
    // This takes care of preventing Drupal's AJAX framework.
    $('form.amazons3-cors-upload-form input.cors-form-submit').unbind('mousedown');

    // Intercept click events for submit buttons in forms with a CORS upload
    // field so we can process the upload to S3 and then actually submit the
    // form when it's all taken care of.
    $('form.amazons3-cors-upload-form input.cors-form-submit', context).one('click', function(e) {
      $form = $(this).parents('form');

      // Disable all the submit buttons, we'll submit the form via JS after
      // file uploads are complete.
      $form.find('input[type="submit"]').attr('disabled', 'disabled');

      var triggering_element = $(this).attr('name');
      AmazonS3corsUpload.handleUpload('.amazons3-cors-upload-file', $form, triggering_element);
      return false;
    });
  };
})(jQuery);

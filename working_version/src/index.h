static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!doctype html>
  <html>
  <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <meta http-equiv="Expires" Content="0">
      <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.0.13/css/all.css"
        integrity="sha384-DNOHZ68U8hZfKXOrtjWvjxusGo9WQnrNx2sqG0tfsghAvtVlRW3tvkXWZh58N9jp" crossorigin="anonymous" />
      <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.1.1/css/bootstrap.min.css"
        integrity="sha384-WskhaSGFgHYWDcbwN70/dfYBj47jz9qbsMId/iRN3ewGhXQFZCSftd1LZCfmhktB" crossorigin="anonymous" />
  </head>

  <body onload="document.getElementById('stream').src=location.origin+':81/stream';">

  <header id="main-header" class="py-2 bg-success text-white">
    <div class="container">
      <div class="row justify-content-md-center">
        <div class="col-md-6 text-center">
          <h1><i class="fas fa-cog"></i> ESP32Cam control ouverture de porte</h1>
        </div>
      </div>
    </div>
  </header>

  <section class="bg-white">
    <div class="container">
      <div class="row">
      <div class="col">
          <div class="card bg-light m-2">
            <div class="card-header">localhost/stream </div>
            <div class="card-body">
              <h5 class="card-title">Camera</h5>
              <img id="stream" style="margin-top: 50px; width:400px"></img><br>

            </div>
          </div>
        </div>
      
      <div class="col">
        <button type="button" class="btn btn-lg btn-success" onclick="fetch(location.origin+'/open?var=relay&val=1');">Relay On</button>
        <button type="button" class="btn btn-lg btn-danger" onclick="fetch(location.origin+'/open?var=relay&val=0');">Relay off</button>
      </div>
      </div>
      
    </div>
  </section>

  </body>

  </html>
)rawliteral";
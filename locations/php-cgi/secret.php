<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>webserv</title>
    <style>
      * {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
      }

      body {
        min-height: 100vh;
        display: flex;
        justify-content: center;
        align-items: center;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        font-family: -apple-system, "SF Mono", Inter, sans-serif;
        padding: 20px;
      }

      .container {
        background: rgba(255, 255, 255, 0.95);
        padding: 3rem 4rem;
        border-radius: 20px;
        box-shadow: 0 20px 40px rgba(0, 0, 0, 0.2);
        text-align: center;
        max-width: 600px;
        width: 100%;
      }

      h1 {
        color: #1a1a1a;
        font-size: 2.5rem;
        margin-bottom: 1rem;
        letter-spacing: -0.5px;
      }

      .status {
        color: #4a5568;
        font-size: 1.2rem;
        line-height: 1.6;
      }

      .badge {
        display: inline-block;
        background: #48bb78;
        color: white;
        padding: 0.5rem 1rem;
        border-radius: 9999px;
        font-size: 0.875rem;
        margin-top: 1.5rem;
        font-weight: 500;
      }

      .server-info {
        margin-top: 2rem;
        padding-top: 2rem;
        border-top: 1px solid #e2e8f0;
        font-size: 0.875rem;
        color: #718096;
      }

      a {
        display: inline-block;
        color: #764ba2;
        text-decoration: none;
        padding: 0.25rem 0.5rem;
        border-radius: 4px;
        transition: all 0.2s ease;
      }

      a:hover {
        background: rgba(118, 75, 162, 0.1);
        text-decoration: none;
        color: #5a3a7e;
      }

      .server-info p {
        margin-block: 1em;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>Webserv/php-cgi</h1>
      <div class="badge">php cgi is up and running</div>
    <div class='status'> Secret Cookie</div>
    <?php
        if ($_SERVER['REQUEST_METHOD'] === 'POST') {
            if (isset($_POST["secret_name"]) && isset($_POST["secret"]))
            {
                setcookie("secret_name", $_POST["secret_name"]);
                setcookie("secret", $_POST["secret"]);
                $_COOKIE['secret_name'] = $_POST["secret_name"];
                $_COOKIE['secret'] = $_POST["secret"];
                echo "<div class='status'>Your secret is saved until you close the browser.</div>";
            }
            else
            {
                echo "<div class='status'>Error secret not saved.</div>";
            }
        }
        if (isset($_COOKIE['secret_name']) && isset($_COOKIE['secret']))
        {
            echo "<h3>Hello mr. ". $_COOKIE['secret_name']."</h3>";
            echo "<h3>Your secret message is:<br>". $_COOKIE['secret']."</h3>";
        }
        else
        {
            echo "<h3>Fill the secret form bellow to save your secret.</h3>";
        }
    ?>
    <div class='status'>Change your secret message:</div>
    <form action="secret.php" method="post">
        <label for="secret_name">Secret Name:</label>
        <input type="text" id="secret_name" name="secret_name"><br><br>
        <label for="secret">Secret:</label>
        <input type="text" id="secret" name="secret"><br><br>
        <input type="submit" value="Submit">
    </form>
    <div class="server-info">
        <p>Built with C++98 | HTTP/1.1 Compliant</p>
        <a href="/">main</a>
        |
        <a href="/php-cgi/php_info.php">php_info</a>
        |
        <a href="https://github.com/obenchkroune/webserv" target="_blank">
          View on GitHub
        </a>
      </div>
    </div>
  </body>
</html>
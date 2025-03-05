#include "esp_attr.h"

#ifdef __cplusplus
extern "C" {
#endif

const char* page_content IRAM_ATTR = R"HTML(
<!DOCTYPE html>
<html>

<head>
    <title>ESP32 ePaper</title>
</head>

<body>
    <div>Counter: <span id="counter"></span></div>
    <br/>
    <div>
        <button onclick="toggleScreen()">Toggle Screen</button>
    </div>
</body>

<script>
    function updateCounter() {
        fetch('/counter')
            .then(response => response.text())
            .then(data => {
                document.getElementById('counter').textContent = data;
            });
    }

    function toggleScreen() {
        fetch('/toggle_screen')
    }

    updateCounter();
</script>
</html>
)HTML";

#ifdef __cplusplus
}
#endif

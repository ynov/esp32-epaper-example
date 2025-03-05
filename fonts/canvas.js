// const font_face = "JetBrains Mono Light";
const font_face = "Ubuntu Mono";
const char_width = 16;
const char_height = 24;
const num_chars = 95;
const dealiasing_intensity = 0x40;
const first_char = 0x20;
const size = char_width * char_height * num_chars;
const size_compact = Math.ceil(size / 8);

const char_arr = [];
for (let i = 0; i < num_chars; i++) {
  char_arr.push(first_char + i);
}

const canvas = document.querySelector("#canvas");
const render_zone = document.querySelector("#render-zone");
const information = document.querySelector(".information");

const binary_array = [];
const binary_array_uint8_compact = [];

function toBinaryStringWithPadding(number, length) {
  return `${"0".repeat(length)}${number.toString(2)}`.slice(-length);
}

function draw() {
  const ctx = canvas.getContext("2d");
  ctx.font = `${char_height}px ${font_face}`;

  canvas.width = char_width * char_arr.length;
  canvas.height = char_height;

  ctx.clearRect(0, 0, canvas.width, canvas.height);

  ctx.font = `${char_height}px ${font_face}`;

  char_arr.forEach((charCode, index) => {
    const char = String.fromCharCode(charCode);
    const x = index * char_width;
    // ctx.fillText(char, x, char_height - 3); // JetBrains Mono Light
    ctx.fillText(char, x, char_height - 5); // Ubuntu Mono
  });

  const image_data = ctx.getImageData(0, 0, canvas.width, canvas.height);
  const pixels = image_data.data;

  console.log("pixels size:", pixels.length);

  // Clear previous binary arrays
  binary_array.length = 0;
  binary_array_uint8_compact.fill(0);

  for (let y = 0; y < canvas.height; y++) {
    for (let x = 0; x < canvas.width; x++) {
      // Each pixel has 4 values (RGBA)
      const index = (y * canvas.width + x) * 4;

      const px =
        pixels[index] +
        pixels[index + 1] +
        pixels[index + 2] +
        pixels[index + 3];

      binary_array.push(px > dealiasing_intensity ? 1 : 0);
    }
  }

  // Compact binary array into uint8 array
  for (let y = 0; y < canvas.height; y++) {
    for (let x = 0; x < canvas.width; x += 8) {
      let byte = 0;
      // Pack 8 bits into one byte
      for (let bit = 0; bit < 8; bit++) {
        if (x + bit < canvas.width) {
          byte = (byte << 1) | binary_array[y * canvas.width + x + bit];
        }
      }
      binary_array_uint8_compact[y * Math.ceil(canvas.width / 8) + x / 8] =
        byte;
    }
  }

  console.log("binary_array length:", binary_array.length);
  console.log("binary_array:", binary_array);
  console.log(
    "binary_array_uint8_compact length:",
    binary_array_uint8_compact.length
  );
  console.log("binary_array_uint8_compact:", binary_array_uint8_compact);

  information.textContent = JSON.stringify(
    {
      font_face,
      char_width,
      char_height,
      num_chars,
      dealiasing_intensity,
      size,
      size_compact,
    },
    null,
    2
  );
}

function render_font() {
  const pre = document.createElement("pre");
  pre.classList.add("render-content");

  const render_array = [];
  for (
    let i = 0;
    i < binary_array_uint8_compact.length;
    i += size_compact / char_height
  ) {
    render_array.push(
      binary_array_uint8_compact
        .slice(i, i + size_compact / char_height)
        .map((x) => toBinaryStringWithPadding(x, 8))
        .join("")
        .replace(/0/g, ".")
        .replace(/1/g, "M")
    );
  }

  pre.textContent = render_array.join("\n");
  render_zone.appendChild(pre);
}

function render_text(
  text,
  array_ref,
  debug = false
) {
  const pre = document.createElement("pre");
  pre.classList.add("render-content");

  const render_array = new Array(char_height)
    .fill(0)
    .map(() => new Array(text.length * char_width).fill(0));

  for (let i = 0; i < text.length; i++) {
    const letter = text[i];
    const char_code = letter.charCodeAt(0);
    const char_index = char_code - first_char;

    if (debug) {
      console.log("debug:", { char_code, char_index, letter });
    }

    for (let y = 0; y < char_height; y++) {
      for (let x = 0; x < char_width; x++) {
        const current_bit = char_index * char_width + x;

        const byte_index =
          y * Math.ceil(size_compact / char_height) +
          Math.floor(current_bit / 8);
        const bit_position = 7 - (current_bit % 8);
        const bit = (array_ref[byte_index] >> bit_position) & 1;

        if (debug) {
          console.log("debug:", { current_bit, byte_index, bit_position, bit });
        }

        render_array[y][x + i * char_width] = bit ? 1 : 0;
      }
    }
  }

  pre.textContent = render_array
    .map((row) => row.join("").replace(/0/g, ".").replace(/1/g, "M"))
    .join("\n");
  render_zone.appendChild(pre);
}

draw();

render_font();
render_text("Hello, woRld! This is coOLq! 123#$%^&*()", window.ubuntu_mono_16x24);
render_text("Hello, woRld! This is coOLq! 123#$%^&*()", window.jetbrains_mono_light_16x24);
render_text("0qRt4!$ $999.5", window.ubuntu_mono_16x24);
render_text("0qRt4!$ $999.5", window.jetbrains_mono_light_16x24);
// render_text("H", window.ubuntu_mono_16x24, true);

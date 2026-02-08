# Deploying GraphAlgo Visualizer to the Web

Since this is a client-side web application (HTML/CSS/JS/Wasm), it can be deployed for free on many static hosting services.

## Recommended Option: Netlify Drop (Easiest)

1.  **Prepare your files**:
    Ensure you have the following files in a single folder:
    -   `index.html`
    -   `style.css`
    -   `script.js`
    -   `dijkstra.js`
    -   `dijkstra.wasm`

2.  **Deploy**:
    -   Go to [https://app.netlify.com/drop](https://app.netlify.com/drop)
    -   Drag and drop the folder containing your files into the upload area.
    -   Netlify will upload and publish your site immediately.

3.  **Done!**
    -   You will get a public URL (e.g., `https://relaxed-babbage-xxxx.netlify.app`).
    -   You can share this link with anyone.

## Option 2: GitHub Pages (Standard for code)

1.  Create a new repository on GitHub.
2.  Upload your files (`index.html`, `style.css`, `script.js`, `dijkstra.js`, `dijkstra.wasm`) to the repository.
3.  Go to `Settings` -> `Pages`.
4.  Under "Source", select `main` branch and `/ (root)` folder.
5.  Click `Save`.
6.  Your site will be live at `https://yourusername.github.io/your-repo-name/`.

## Option 3: Vercel

1.  Go to [Vercel.com](https://vercel.com).
2.  Install Vercel CLI or import from your GitHub repository.
3.  If importing from GitHub, simply select the repo and click "Deploy". Vercel auto-detects static sites.

---

**Important Note regarding WebAssembly (.wasm)**:
Ensure your web server (or hosting provider) serves `.wasm` files with the correct MIME type: `application/wasm`. Netlify and GitHub Pages handle this automatically.

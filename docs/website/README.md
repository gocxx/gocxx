# gocxx Website

This directory contains the single-page website for gocxx, deployed automatically to GitHub Pages.

## 🌐 Live Site

The website is automatically deployed to: `https://gocxx.github.io/gocxx/`

## 🎨 Features

- **Modern Design**: Clean, dark theme with gradient accents
- **Responsive**: Works perfectly on desktop, tablet, and mobile
- **Interactive**: Smooth scrolling, hover effects, and animations
- **Performance**: Single-page, minimal dependencies, fast loading
- **SEO Optimized**: Meta tags, OpenGraph, and Twitter cards

## 🚀 Sections

1. **Hero**: Eye-catching introduction with call-to-action buttons
2. **Features**: Key benefits and capabilities of gocxx
3. **Code Examples**: Live syntax-highlighted code samples
4. **Modules**: Complete overview of all available modules
5. **Getting Started**: Step-by-step installation guide
6. **Footer**: Links to documentation, GitHub, and resources

## 🛠️ Local Development

To test the website locally:

```bash
# Simple HTTP server (Python)
cd docs/website
python -m http.server 8000

# Or with Node.js
npx serve .

# Then open http://localhost:8000
```

## 📦 Deployment

The website is automatically deployed via GitHub Actions when:
- Changes are pushed to the `main` branch in `docs/website/`
- Manual workflow trigger

The deployment workflow:
1. Copies website files to the build directory
2. Optionally generates API documentation with Doxygen
3. Deploys to GitHub Pages

## 🎯 Customization

The website uses CSS custom properties (variables) for easy theming:

```css
:root {
    --primary: #007acc;          /* Main brand color */
    --secondary: #00d4aa;        /* Accent color */
    --background: #0f1419;       /* Dark background */
    --surface: #1a1f29;          /* Card backgrounds */
    --text: #ffffff;             /* Primary text */
    --text-muted: #8892b0;       /* Secondary text */
}
```

## 📝 Content Updates

To update website content:

1. Edit `index.html` directly
2. Commit and push to `main` branch
3. GitHub Actions will automatically deploy

## 🔗 Links

- **Live Site**: https://gocxx.github.io/gocxx/
- **Repository**: https://github.com/gocxx/gocxx
- **Documentation**: https://gocxx.github.io/gocxx/api/
- **Examples**: https://github.com/gocxx/gocxx/tree/main/examples

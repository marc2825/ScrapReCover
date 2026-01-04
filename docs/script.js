function setLang(lang) {
    const body = document.body;
    const btns = document.querySelectorAll('.lang-btn');
    
    // Toggle body class
    if (lang === 'jp') {
        body.classList.add('lang-jp');
        body.classList.remove('lang-en');
    } else {
        body.classList.add('lang-en');
        body.classList.remove('lang-jp');
    }

    btns.forEach(btn => {
        if (btn.textContent.toLowerCase() === lang) {
            btn.classList.add('active');
        } else {
            btn.classList.remove('active');
        }
    });
}
location.href = "https://github.com/tree-sitter/tree-sitter/wiki/List-of-parsers"

list = Array.from(document.querySelector("#wiki-body > div > table > tbody").children).map(n => {
    const grammarJson=n.children[4].innerText.trim()
    const externalScanner=n.children[5].innerText.trim()
    return {
        name:n.children[0].innerText.trim(),
        url:n.children[1].querySelector("a").href,
        date:n.children[2].innerText.trim(),
        abi:n.children[3].innerText.trim(),
        grammarJson:grammarJson=='yes'?true:grammarJson=='no'?false:grammarJson,
        externalScanner:externalScanner=='yes'?true:externalScanner=='no'?false:externalScanner,
        download:false
    }
})

console.log(JSON.stringify(list,null,' '))

console.log(list.map((n,i) => Object.values(n).join(";")).join("\n"))
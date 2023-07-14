$(document).foundation();

const schemes = await d3.csv("data/schemes.csv", (d) => {
  return {
    Scheme: d.Scheme,
    Status: d["NIST status"],
    Website: d.Website,
    Category: d.Category,
    Assumption: d.Assumption,
  };
});
const properties = await d3.csv("data/parametersets.csv", (d) => {
  return {
    Scheme: d.Scheme,
    Parameterset: d.Parameterset,
    Level: +d["Security level"],
    Pk: +d["pk size"].replace(/,/g, ""),
    Sig: +d["sig size"].replace(/,/g, ""),
  };
});
const categories = new Set(schemes.map((s) => s.Category));

const table = d3.select("#scheme-table");
const propsTable = d3.select("#properties-table");

function cleanId(name) {
    return name.replace(/[^a-zA-Z0-9]/, '_');
}

function reenableCategoryForScheme(event, scheme) {
    if (!event.target.checked){
        return;
    }
    schemes.forEach(s => {
        console.log(s, scheme);
        if (s.Scheme === scheme) {
            d3.select("#switch-" + cleanId(s.Category)).property("checked", true);
            return;
        }
    });
}

function updateTable(event) {
  console.log("updating tables");
  const selectedCategories = d3.selectAll(".category > input:checked").data();

  schemes.forEach((s) => {
    if (!selectedCategories.includes(s.Category)) {
        d3.select("#switch-" + cleanId(s.Scheme)).property("checked", false);
    }
  })

  // data
  table
    .select("tbody")
    .selectAll("tr")
    .data(
      schemes.filter((s) => selectedCategories.includes(s.Category)),
      (d) => d.Scheme
    )
    .join((enter) =>
      enter.append((d) => {
        const row = d3.create("tr").property("data-scheme", d.Scheme);
        row
          .append("td")
          .append("a")
          .attr("href", d.Website)
          .attr("target", "_blank")
          .text(d.Scheme);
        row.append("td").text(d.Status);
        row.append("td").text(d.Category);
        row.append("td").text(d.Assumption);

        return row.node();
      })
    );

  let selectedSchemes = schemes.map((d) => d.Scheme);
    if (event !== undefined) {
      selectedSchemes = d3
        .selectAll("#schemes-filter input:checked")
        .data();
    }

    const selectedLevels = d3.selectAll("#levels-filter input:checked").data();

  const propRows = propsTable
    .select("tbody")
    .selectAll("tr")
    .data(
      properties.filter((p) => selectedSchemes.includes(p.Scheme) && selectedLevels.includes(p.Level)),
      (d) => d.Scheme + d.Parameterset
    )
    .join((enter) =>
      enter.append((d) => {
        const row = d3.create("tr");

        row.append("td").text(d.Scheme);
        row.append("td").text(d.Parameterset);
        row.append("td").text(d.Level);
        row.append("td").text(d.Pk.toLocaleString());
        row.append("td").text(d.Sig.toLocaleString());
        row.append("td").text((d.Pk + d.Sig).toLocaleString())
        return row.node();
      })
    );
}

d3.select("#categories")
  .selectAll("div")
  .classed("grid-x", true)
  .data(categories)
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true)
      .classed("category", true);
    toggle
      .append("input")
      .attr("type", "checkbox")
      .attr("id", "switch-" + cleanId(d))
      .classed("switch-input categories-filter", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", updateTable);
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "switch-" + cleanId(d))
      .append((e) =>
        d3
          .create("span")
          .classed("show-for-sr", true)
          .text("Show/hide " + d)
          .node()
      );
    cat.append("span").classed("cell auto", true).text(d);

    return cat.node();
  });

d3.select("#schemes-filter")
  .selectAll("div")
  .classed("grid-x", true)
  .data(schemes.map((s) => s.Scheme))
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true)
    toggle
      .append("input")
      .attr("type", "checkbox")
      .classed("scheme-filter", true)
      .attr("id", "switch-" + cleanId(d))
      .classed("switch-input", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", (e) => {
        reenableCategoryForScheme(e, d);
        updateTable(e);
    });
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "switch-" + cleanId(d))
      .append((e) =>
        d3
          .create("span")
          .classed("show-for-sr", true)
          .text("Show/hide " + d)
          .node()
      );
    cat.append("span").classed("cell auto", true).text(d);

    return cat.node();
  });

d3.select("#levels-filter")
  .selectAll("div")
  .classed("grid-x", true)
  .data([1, 2, 3, 4, 5])
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true)
    toggle
      .append("input")
      .attr("type", "checkbox")
      .classed("nistlevel-filter", true)
      .attr("id", "switch-level-" + d)
      .classed("switch-input", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", (e) => {
        updateTable(e);
    });
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "switch-level-" + d)
      .append((e) =>
        d3
          .create("span")
          .classed("show-for-sr", true)
          .text("Show/hide NIST level " + d)
          .node()
      );
    cat.append("span").classed("cell auto", true).text("Level " + d);

    return cat.node();
  });

d3.select("#select-all-params").on('click', (e) => {
    d3.selectAll(".scheme-filter").property("checked", true)
    d3.selectAll(".categories-filter").property("checked", true)
    updateTable(e)
})
d3.select("#select-none-params").on('click', (e) => {
    d3.selectAll(".scheme-filter").property("checked", false)
    updateTable(e)
})

updateTable();
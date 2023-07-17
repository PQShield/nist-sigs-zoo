$(document).foundation();

const CPUSPEED = 2_500_000_000;

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
  let signcycles;
  let verifycycles;
  let extrapolated;
  if (parseInt(d["signing (cycles)"].replace(/,/g, "")) > 0 ) {
    extrapolated = false;
    signcycles = parseInt(d["signing (cycles)"].replace(/,/g, ""));
    verifycycles = parseInt(d["verification (cycles)"].replace(/,/g, ""))
  } else {
    extrapolated = true;
    signcycles = (CPUSPEED * parseFloat(d["signing (ms)"].replace(/,/g, ""))) / 1000;
    verifycycles = (CPUSPEED *parseFloat(d["verification (ms)"].replace(/,/g, ""))) / 1000;
  }

  return {
    Scheme: d.Scheme,
    Parameterset: d.Parameterset,
    Level: +d["Security level"],
    Pk: +d["pk size"].replace(/,/g, ""),
    Sig: +d["sig size"].replace(/,/g, ""),
    PkPlusSig:
      parseFloat(d["pk size"].replace(/,/g, "")) +
      parseInt(d["sig size"].replace(/,/g, "")),
    SigningCycles: signcycles,
    VerificationCycles: verifycycles,
    SigningTime: parseFloat(d["signing (ms)"].replace(/,/g, "")),
    VerificationTime: parseFloat(d["verification (ms)"].replace(/,/g, "")),
    Extrapolated: extrapolated,
  };
});
const categories = new Set(schemes.map((s) => s.Category));

const table = d3.select("#scheme-table");
const propsTable = d3.select("#properties-table");

function cleanId(name) {
  return name.replace(/[^a-zA-Z0-9]/, "_");
}

function reenableCategoryForScheme(event, scheme) {
  if (!event.target.checked) {
    return;
  }
  schemes.forEach((s) => {
    console.log(s, scheme);
    if (s.Scheme === scheme) {
      d3.select("#switch-" + cleanId(s.Category)).property("checked", true);
      return;
    }
  });
}

let schemeSortingDirection = 1;
let nowSortingScheme = "Scheme";
let propertiesSortingDirection = 1;
let nowSortingProperties = "Scheme";
let performanceSortingDirection = 1;
let nowSortingPerformance = "Scheme";

function updateTable(event) {
  console.log("updating tables");
  const selectedCategories = d3.selectAll(".category > input:checked").data();

  schemes.forEach((s) => {
    if (!selectedCategories.includes(s.Category)) {
      d3.select("#switch-" + cleanId(s.Scheme)).property("checked", false);
    }
  });

  function sortAndFilterSchemes() {
    return schemes
      .filter((s) => selectedCategories.includes(s.Category))
      .sort(
        (a, b) =>
          schemeSortingDirection *
          d3.ascending(a[nowSortingScheme], b[nowSortingScheme])
      );
  }

  // data
  table
    .select("tbody")
    .selectAll("tr")
    .data(sortAndFilterSchemes(), (d) => d.Scheme)
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

  let selectedPropsSchemes = schemes.map((d) => d.Scheme);
  if (event !== undefined) {
    selectedPropsSchemes = d3
      .selectAll("#props-schemes-filter input:checked")
      .data();
  }

  const selectedPropsLevels = d3
    .selectAll("#props-levels-filter input:checked")
    .data();

  function sortAndFilterProperties() {
    return properties
      .filter(
        (p) =>
          selectedPropsSchemes.includes(p.Scheme) &&
          selectedPropsLevels.includes(p.Level)
      )
      .sort(
        (a, b) =>
          propertiesSortingDirection *
          d3.ascending(a[nowSortingProperties], b[nowSortingProperties])
      );
  }

  d3.select("#properties-table")
    .select("tbody")
    .selectAll("tr")
    .data(sortAndFilterProperties(), (d) => d.Scheme + d.Parameterset)
    .join((enter) =>
      enter.append((d) => {
        const row = d3.create("tr");

        row.append("td").text(d.Scheme);
        row.append("td").text(d.Parameterset);
        row.append("td").text(d.Level);
        row
          .append("td")
          .text(d.Pk.toLocaleString())
          .attr("style", "text-align: right");
        row
          .append("td")
          .text(d.Sig.toLocaleString())
          .attr("style", "text-align: right");
        row
          .append("td")
          .text(d.PkPlusSig.toLocaleString())
          .attr("style", "text-align: right");
        return row.node();
      })
    );

  let selectedPerfSchemes = schemes.map((d) => d.Scheme);
  if (event !== undefined) {
    selectedPerfSchemes = d3
      .selectAll("#perf-schemes-filter input:checked")
      .data();
  }

  const selectedPerfLevels = d3
    .selectAll("#perf-levels-filter input:checked")
    .data();

  function sortAndFilterPerformance() {
    return properties
      .filter(
        (p) =>
          selectedPerfSchemes.includes(p.Scheme) &&
          selectedPerfLevels.includes(p.Level)
      )
      .sort(
        (a, b) =>
          performanceSortingDirection *
          d3.ascending(a[nowSortingPerformance], b[nowSortingPerformance])
      );
  }

  d3.select("#performance-table")
    .select("tbody")
    .selectAll("tr")
    .data(sortAndFilterPerformance(), (d) => d.Scheme + d.Parameterset)
    .join((enter) =>
      enter.append((d) => {
        const row = d3.create("tr");

        row.append("td").text(d.Scheme);
        row.append("td").text(d.Parameterset);
        row.append("td").text(d.Level);
        let extrapolated_text_sign;
        let extrapolated_text_verify;
        if (d.Extrapolated) {
          extrapolated_text_sign =
            "Reported as " + d.SigningTime.toLocaleString() + " ms.";
          extrapolated_text_verify =
            "Reported as " + d.VerificationTime.toLocaleString() + " ms.";
        }
        row
          .append("td")
          .text(d.SigningCycles.toLocaleString())
          .classed("extrapolated", d.Extrapolated)
          .property("title", extrapolated_text_sign)
          .attr("style", "text-align: right");
        row
          .append("td")
          .text(d.VerificationCycles.toLocaleString())
          .classed("extrapolated", d.Extrapolated)
          .property("title", extrapolated_text_verify)
          .attr("style", "text-align: right");
        return row.node();
      })
    );
}

d3.select("#categories")
  .selectAll("div")
  .classed("grid-x", true)
  .data([...categories].sort(d3.ascending))
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

d3.select("#props-schemes-filter")
  .selectAll("div")
  .classed("grid-x", true)
  .data(schemes.map((s) => s.Scheme).sort(d3.ascending))
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true);
    toggle
      .append("input")
      .attr("type", "checkbox")
      .classed("scheme-filter", true)
      .attr("id", "props-switch-" + cleanId(d))
      .classed("switch-input", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", (e) => {
        d3.select("#perf-switch-" + cleanId(d)).property(
          "checked",
          e.target.checked
        );
        reenableCategoryForScheme(e, d);
        updateTable(e);
      });
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "props-switch-" + cleanId(d))
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

d3.select("#props-levels-filter")
  .selectAll("div")
  .classed("grid-x", true)
  .data([1, 2, 3, 4, 5])
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true);
    toggle
      .append("input")
      .attr("type", "checkbox")
      .classed("nistlevel-filter", true)
      .attr("id", "props-switch-level-" + d)
      .classed("switch-input", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", (e) => {
        updateTable(e);
      });
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "props-switch-level-" + d)
      .append((e) =>
        d3
          .create("span")
          .classed("show-for-sr", true)
          .text("Show/hide NIST level " + d)
          .node()
      );
    cat
      .append("span")
      .classed("cell auto", true)
      .text("Level " + d);

    return cat.node();
  });

d3.select("#perf-levels-filter")
  .selectAll("div")
  .classed("grid-x", true)
  .data([1, 2, 3, 4, 5])
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true);
    toggle
      .append("input")
      .attr("type", "checkbox")
      .classed("nistlevel-filter", true)
      .attr("id", "perf-switch-level-" + d)
      .classed("switch-input", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", (e) => {
        updateTable(e);
      });
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "perf-switch-level-" + d)
      .append((e) =>
        d3
          .create("span")
          .classed("show-for-sr", true)
          .text("Show/hide NIST level " + d)
          .node()
      );
    cat
      .append("span")
      .classed("cell auto", true)
      .text("Level " + d);

    return cat.node();
  });

d3.select("#perf-schemes-filter")
  .selectAll("div")
  .classed("grid-x", true)
  .data(schemes.map((s) => s.Scheme).sort(d3.ascending))
  .enter()
  .append((d) => {
    const cat = d3.create("div").classed("grid-x", true);

    const toggle = cat
      .append("div")
      .classed("cell small-3", true)
      .classed("switch tiny", true);
    toggle
      .append("input")
      .attr("type", "checkbox")
      .classed("scheme-filter", true)
      .attr("id", "perf-switch-" + cleanId(d))
      .classed("switch-input", true)
      .property("checked", "checked")
      .datum(d)
      .on("click", (e) => {
        d3.select("#props-switch-" + cleanId(d)).property(
          "checked",
          e.target.checked
        );
        reenableCategoryForScheme(e, d);
        updateTable(e);
      });
    toggle
      .append("label")
      .classed("switch-paddle", true)
      .attr("for", "perf-switch-" + cleanId(d))
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

d3.selectAll(".select-all-params").on("click", (e) => {
  d3.selectAll(".scheme-filter").property("checked", true);
  d3.selectAll(".categories-filter").property("checked", true);
  updateTable(e);
});
d3.selectAll(".select-none-params").on("click", (e) => {
  d3.selectAll(".scheme-filter").property("checked", false);
  updateTable(e);
});

updateTable();

function handleSortingSchemes(what) {
  return (e) => {
    if (nowSortingScheme === what) {
      schemeSortingDirection *= -1;
    } else {
      schemeSortingDirection = 1;
      nowSortingScheme = what;
    }
    updateTable(e);
  };
}

function handleSortingProperties(what) {
  return (e) => {
    if (nowSortingProperties === what) {
      propertiesSortingDirection *= -1;
    } else {
      propertiesSortingDirection = 1;
      nowSortingProperties = what;
    }
    updateTable(e);
  };
}

function handleSortingPerformance(what) {
  return (e) => {
    if (nowSortingPerformance === what) {
      performanceSortingDirection *= -1;
    } else {
      performanceSortingDirection = 1;
      nowSortingPerformance = what;
    }
    updateTable(e);
  };
}

d3.select("#header-schemes-scheme").on("click", handleSortingSchemes("Scheme"));
d3.select("#header-schemes-status").on("click", handleSortingSchemes("Status"));
d3.select("#header-schemes-category").on(
  "click",
  handleSortingSchemes("Category")
);
d3.select("#header-schemes-assumption").on(
  "click",
  handleSortingSchemes("Assumption")
);

d3.select("#header-properties-scheme").on(
  "click",
  handleSortingProperties("Scheme")
);
d3.select("#header-properties-parameterset").on(
  "click",
  handleSortingProperties("Parameterset")
);
d3.select("#header-properties-level").on(
  "click",
  handleSortingProperties("Level")
);
d3.select("#header-properties-pk").on("click", handleSortingProperties("Pk"));
d3.select("#header-properties-sig").on("click", handleSortingProperties("Sig"));
d3.select("#header-properties-pksig").on(
  "click",
  handleSortingProperties("PkPlusSig")
);


d3.select("#header-performance-scheme").on("click", handleSortingPerformance("Scheme"));
d3.select("#header-performance-parameterset").on("click", handleSortingPerformance("Parameterset"));
d3.select("#header-performance-level").on("click", handleSortingPerformance("Level"));
d3.select("#header-performance-sign").on("click", handleSortingPerformance("SigningCycles"));
d3.select("#header-performance-verify").on("click", handleSortingPerformance("VerificationCycles"));
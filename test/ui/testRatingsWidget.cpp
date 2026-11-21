#include "third_party/catch2/catch.hpp"

#include "data/Rating.h"
#include "model/RatingModel.h"
#include "ui/small_widgets/RatingsWidget.h"

#include <QAbstractButton>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTableView>
#include <QTest>

TEST_CASE("Ratings editor adds, edits and removes the selected rating", "[ui][ratings]")
{
    Ratings ratings;
    Rating original;
    original.source = "imdb";
    original.rating = 7.0;
    ratings.addRating(original);

    RatingsWidget widget;
    widget.setRatings(&ratings);
    widget.show();

    auto* add = widget.findChild<QAbstractButton*>("btnAddRating");
    auto* remove = widget.findChild<QAbstractButton*>("btnRemoveRating");
    auto* table = widget.findChild<QTableView*>("ratings");
    REQUIRE(add);
    REQUIRE(remove);
    REQUIRE(table);
    QSignalSpy changes(&widget, &RatingsWidget::ratingsChanged);

    QTest::mouseClick(add, Qt::LeftButton);
    REQUIRE(ratings.size() == 2);
    REQUIRE(table->selectionModel()->isRowSelected(1, {}));
    CHECK(changes.count() == 1);

    const auto index = table->model()->index(1, RatingModel::RatingColumn);
    const auto cell = table->visualRect(index).center();
    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, cell);
    QTest::mouseDClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, cell);
    auto* spinBox = table->findChild<QDoubleSpinBox*>();
    REQUIRE(spinBox);
    auto* editor = spinBox->findChild<QLineEdit*>();
    REQUIRE(editor);
    QTest::keyClick(editor, Qt::Key_A, Qt::ControlModifier);
    QTest::keyClicks(editor, "8");
    QTest::keyClick(editor, Qt::Key_Return);
    QApplication::processEvents();
    CHECK(index.data(Qt::EditRole).toDouble() == 8.0);
    CHECK(ratings.at(1).rating == 8.0);
    CHECK(changes.count() == 2);

    QTest::mouseClick(remove, Qt::LeftButton);
    REQUIRE(ratings.size() == 1);
    CHECK(ratings.first().source == original.source);
    CHECK(ratings.first().rating == original.rating);
    CHECK(changes.count() == 3);

    table->clearSelection();
    QTest::mouseClick(remove, Qt::LeftButton);
    CHECK(ratings.size() == 1);
    CHECK(changes.count() == 3);
}
